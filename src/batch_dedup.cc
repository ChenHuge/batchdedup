#include "mpi.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include "trace_types.h"
#include "batch_dedup_config.h"
#include "mpi_engine.h"
#include "snapshot_mixer.h"
#include "partition_index.h"
#include "dedup_buffer.h"
#include "snapshot_meta.h"
#include "timer.h"
#include "unistd.h"

using namespace std;

void usage(char* progname)
{
    cout << "Usage: " << progname << " total_num_partitions total_num_VMs num_snapshots mpi_buffer read_buffer write_buffer" << endl;
    return;
}

// setup environment, create directories
void init(int argc, char** argv)
{
    int num_tasks, rank;
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    Env::SetNumTasks(num_tasks);
    Env::SetRank(rank);
    Env::SetNumPartitions(atoi(argv[1]));
    Env::SetNumVms(atoi(argv[2]));
    Env::SetNumSnapshots(atoi(argv[3]));
    Env::SetMpiBufSize(atoi(argv[4]) * 1024);
    Env::SetReadBufSize(atoi(argv[5]) * 1024);
    Env::SetWriteBufSize(atoi(argv[6]) * 1024);
    // triton settings
    Env::SetRemotePath("/oasis/triton/scratch/wei-ucsb/");
    Env::SetLocalPath("/state/partition1/batchdedup/");
    Env::SetHomePath("/home/wei-ucsb/batchdedup/");
    // lonestar settings
    // Env::SetRemotePath("/scratch/02292/mraway/");
    // Env::SetLocalPath("/tmp/batchdedup/");
    // Env::SetHomePath("/work/02292/mraway/batchdedup/")

    if (Env::GetRank() == 0) {
        Env::InitDirs();
    }
    MPI_Barrier(MPI_COMM_WORLD);	// other process wait for shared dirs to be created
    Env::SetLogger();
    Env::LoadSampleTraceList("/home/wei-ucsb/batchdedup/sample_traces");
    LOG_INFO(Env::ToString());
}

void final()
{
#ifdef DEBUG
    stringstream ss;
    ss << "cp -r " << Env::GetLocalPath() << "* " << Env::GetRemotePath();
    system(ss.str().c_str());
#endif

    Env::RemoveLocalPath();
    Env::CloseLogger();
}

int main(int argc, char** argv)
{
    if (argc != 7) {
        usage(argv[0]);
        return 0;
    }

    TimerPool::Start("Total");
    init(argc, argv);
    // cannot allocate 2d array because MPI only accepts one continous buffer
    char* send_buf  = new char[Env::GetNumTasks() * Env::GetMpiBufSize()];
    char* recv_buf  = new char[Env::GetNumTasks() * Env::GetMpiBufSize()];
    Env::SetSendBuf(send_buf);
    Env::SetRecvBuf(recv_buf);

    // avoid too many concurrent access to lustre
    sleep(5 * Env::GetRank());

    LOG_INFO("preparing traces");
    TimerPool::Start("PrepareTrace");
    // local-1: prepare traces, partition indices
    int i = 0;
    while (Env::GetVmId(i) >= 0) {
        int vmid = Env::GetVmId(i);
        int ssid = Env::GetNumSnapshots();
        string source_trace = Env::GetSampleTrace(vmid);
        string mixed_trace = Env::GetVmTrace(vmid);
        SnapshotMixer mixer(vmid, ssid, source_trace, mixed_trace);
        mixer.Generate();
        i++;
    }
    TimerPool::Stop("PrepareTrace");    

    LOG_INFO("loading partition index from lustre");
    TimerPool::Start("LoadIndex");
    for (i = Env::GetPartitionBegin(); i < Env::GetPartitionEnd(); i++) {
        string remote_fname = Env::GetRemoteIndexName(i);
        string local_fname = Env::GetLocalIndexName(i);
        stringstream cmd;
        if (Env::FileExists(remote_fname)) {
            cmd << "cp " << remote_fname << " " << local_fname;
            system(cmd.str().c_str());
        }
    }
    TimerPool::Stop("LoadIndex");

    LOG_INFO("exchanging dirty blocks");
    TimerPool::Start("ExchangeDirtyBlocks");
    // mpi-1: exchange dirty segments
    do {
        MpiEngine* p_mpi = new MpiEngine();
        TraceReader* p_reader = new TraceReader();
        RawRecordAccumulator* p_accu = new RawRecordAccumulator();

        p_mpi->SetDataSpout(dynamic_cast<DataSpout*>(p_reader));
        p_mpi->SetDataSink(dynamic_cast<DataSink*>(p_accu));
        p_mpi->Start();

        delete p_mpi;
        delete p_reader;
        delete p_accu;
    } while(0);
    TimerPool::Stop("ExchangeDirtyBlocks");
    /*
    LOG_INFO("making dedup comparison");
    TimerPool::Start("DedupComparison");
    // local-2: compare with partition index
    for (int partid = Env::GetPartitionBegin(); partid < Env::GetPartitionEnd(); partid++) {
        PartitionIndex index;
        index.FromFile(Env::GetLocalIndexName(partid));
        RecordReader<Block> reader(Env::GetStep2InputName(partid));
        Block blk;
        BlockMeta bm;
        DedupBuffer buf;
        RecordWriter<BlockMeta> output1(Env::GetStep2Output1Name(partid));
        RecordWriter<Block> output2(Env::GetStep2Output2Name(partid));
        RecordWriter<Block> output3(Env::GetStep2Output3Name(partid));

        while(reader.Get(blk)) {
            // dup with existing blocks?
            if (index.Find(blk.mCksum)) {
                bm.mBlk = blk;
                bm.mRef = REF_VALID;
                output1.Put(bm);
                continue;
            }
            // dup with new blocks in this run?
            if (buf.Find(blk)) {
                output2.Put(blk);
                continue;
            }
            // completely new
            buf.Add(blk);
            output3.Put(blk);
        }
    }    
    TimerPool::Stop("DedupComparison");
    Env::StatPartitionIndexSize();

    LOG_INFO("exchange new blocks");
    TimerPool::Start("ExchangeNewBlocks");
    // mpi-2: exchange new blocks
    do {
        MpiEngine* p_mpi = new MpiEngine();
        NewBlockReader* p_reader = new NewBlockReader();
        NewRecordAccumulator* p_accu = new NewRecordAccumulator();

        p_mpi->SetDataSpout(dynamic_cast<DataSpout*>(p_reader));
        p_mpi->SetDataSink(dynamic_cast<DataSink*>(p_accu));
        p_mpi->Start();

        delete p_mpi;
        delete p_reader;
        delete p_accu;
    } while (0);
    TimerPool::Stop("ExchangeNewBlocks");    

    LOG_INFO("writing new blocks to backup storage");
    TimerPool::Start("WriteNewBlocks");
    // local-3: write new blocks to storage
    for (i = 0; Env::GetVmId(i) >= 0; i++) {
        int vmid = Env::GetVmId(i);
        RecordReader<Block> input(Env::GetStep3InputName(vmid));
        RecordWriter<BlockMeta> output(Env::GetStep3OutputName(vmid));
        Block blk;
        BlockMeta bm;
        while (input.Get(blk)) {
            bm.mBlk = blk;
            bm.mRef = REF_VALID;
            output.Put(bm);
        }
    }
    TimerPool::Stop("WriteNewBlocks");

    LOG_INFO("exchanging data reference of new blocks");
    TimerPool::Start("ExchangeNewRefs");
    // mpi-3: exchange new block ref
    do {
        MpiEngine* p_mpi = new MpiEngine();
        NewRefReader* p_reader = new NewRefReader();
        NewRefAccumulator* p_accu = new NewRefAccumulator();

        p_mpi->SetDataSpout(dynamic_cast<DataSpout*>(p_reader));
        p_mpi->SetDataSink(dynamic_cast<DataSink*>(p_accu));
        p_mpi->Start();

        delete p_mpi;
        delete p_reader;
        delete p_accu;
    } while (0);
    TimerPool::Stop("ExchangeNewRefs");

    LOG_INFO("updating partition index and dup_new block references");
    TimerPool::Start("UpdateRefAndIndex");
    // local-4: update refs to pending blocks, then update partition index
    for (int partid = Env::GetPartitionBegin(); partid < Env::GetPartitionEnd(); partid++) {
        PartitionIndex index;
        index.FromFile(Env::GetStep4InputName(partid));
        RecordReader<Block> input(Env::GetStep2Output2Name(partid));
        RecordWriter<BlockMeta> output(Env::GetStep2Output1Name(partid), true);
        Block blk;
        BlockMeta bm;
        while (input.Get(blk)) {
            if (index.Find(blk.mCksum)) {
                bm.mBlk = blk;
                bm.mRef = REF_VALID;
                output.Put(bm);
            }
            else {
                LOG_ERROR("cannot find the ref for a pending block");
            }
        }
        index.AppendToFile(Env::GetLocalIndexName(partid));
    }
    TimerPool::Stop("UpdateRefAndIndex");

    LOG_INFO("exchanging dup blocks");
    TimerPool::Start("ExchangeDupBlocks");
    // mpi-4: exchange dup block meta
    do {
        MpiEngine* p_mpi = new MpiEngine();
        DupBlockReader* p_reader = new DupBlockReader();
        DupRecordAccumulator* p_accu = new DupRecordAccumulator();

        p_mpi->SetDataSpout(dynamic_cast<DataSpout*>(p_reader));
        p_mpi->SetDataSink(dynamic_cast<DataSink*>(p_accu));
        p_mpi->Start();

        delete p_mpi;
        delete p_reader;
        delete p_accu;
    } while (0);
    TimerPool::Stop("ExchangeDupBlocks");

    LOG_INFO("uploading partition index to lustre");
    TimerPool::Start("UploadIndex");
    for (i = Env::GetPartitionBegin(); i < Env::GetPartitionEnd(); i++) {
        string remote_fname = Env::GetRemoteIndexName(i);
        string local_fname = Env::GetLocalIndexName(i);
        stringstream cmd;
        cmd << "cp " << local_fname << " " << remote_fname;
        system(cmd.str().c_str());
    }
    TimerPool::Stop("UploadIndex");
    */
    TimerPool::Stop("Total");
    TimerPool::PrintAll();

    // clean up
    delete[] send_buf;
    delete[] recv_buf;
    final();
    MPI_Finalize();
    return 0;
}


















