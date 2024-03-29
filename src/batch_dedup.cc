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
#include "cpu_usage.h"

using namespace std;

void usage(char* progname)
{
    cout << "Usage: " << progname << " <arguments>" << endl
         << "Where arguments contains one of each of:" << endl
         << "  --partitions <partition_count>" << endl
         << "  --nodevms <vms_per_node>" << endl
         << "  --ssid <current_snapshot_number>" << endl
         << "  --mpibufsize <mpi_buffer_size> (in KB)" << endl
         << "  --rbufsize <read_buffer_size> (in KB)" << endl
         << "  --wbufsize <write_buffer_size> (in KB)" << endl
         << "  --remotepath <path>" << endl
         << "  --localpath <path>" << endl
         << "  --homepath <path>" << endl
         << "  --snapshotfile <initial_snapshot_list_file>" << endl;

    return;
}

// setup environment, create directories
void init(int argc, char** argv)
{
    int num_tasks, rank, argi;
    char* snapshot_file;
    bool partitions_set = false;
    bool numvms_set = false;
    bool ssid_set = false;
    bool mpibufsize_set = false;
    bool readbufsize_set = false;
    bool writebufsize_set = false;
    bool remotepath_set = false;
    bool localpath_set = false;
    bool homepath_set = false;
    bool snapshotfile_set = false;
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    Env::SetNumTasks(num_tasks);
    Env::SetRank(rank);
    argi = 1;
    while(argi < argc && argv[argi][0] == '-') {
        if (strcmp(argv[argi],"--partitions") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No partition count given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetNumPartitions(atoi(argv[argi++]));
            partitions_set = true;
        } else if (strcmp(argv[argi],"--nodevms") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No vm count given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetNumVms(atoi(argv[argi++]));
            numvms_set = true;
        } else if (strcmp(argv[argi],"--ssid") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No ssid given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetNumSnapshots(atoi(argv[argi++]));
            ssid_set = true;
        } else if (strcmp(argv[argi],"--mpibufsize") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No mpi buffer size given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetMpiBufSize(atoi(argv[argi++]) * 1024);
            mpibufsize_set = true;
        } else if (strcmp(argv[argi],"--rbufsize") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No read buffer size given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetReadBufSize(atoi(argv[argi++]) * 1024);
            readbufsize_set = true;
        } else if (strcmp(argv[argi],"--wbufsize") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No write buffer size given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetWriteBufSize(atoi(argv[argi++]) * 1024);
            writebufsize_set = true;
        } else if (strcmp(argv[argi],"--remotepath") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No remote path given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetRemotePath(argv[argi++]);
            remotepath_set = true;
        } else if (strcmp(argv[argi],"--localpath") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No local path given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetLocalPath(argv[argi++]);
            localpath_set = true;
        } else if (strcmp(argv[argi],"--homepath") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No home path given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetHomePath(argv[argi++]);
            homepath_set = true;
        } else if (strcmp(argv[argi],"--snapshotfile") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No snapshot file path given" << endl;
                usage(argv[0]);
                exit(1);
            }
            snapshot_file = argv[argi++];
            snapshotfile_set = true;
        } else if (strcmp(argv[argi],"--timelimit") == 0) {
            argi++;
            if (argi >= argc) {
                cout << "No time limit given" << endl;
                usage(argv[0]);
                exit(1);
            }
            Env::SetTimeLimit(atoi(argv[argi++]));
        } else if (strcmp(argv[argi],"-?") == 0) {
            usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[argi],"--") == 0) {
            argi++;
            cout << "-- break" << endl;
            break;
        } else {
            stringstream ss;
            ss << "Unknown option given: " << argv[argi++];
            cout << ss.str().c_str() << endl;
            usage(argv[0]);
            exit(1);
        }
    }
    // triton settings
    // lonestar settings
    // Env::SetRemotePath("/scratch/02292/mraway/");
    // Env::SetLocalPath("/tmp/batchdedup/");
    // Env::SetHomePath("/work/02292/mraway/batchdedup/")
    if (!partitions_set || !numvms_set || !ssid_set || !mpibufsize_set ||
        !readbufsize_set || !writebufsize_set || !remotepath_set ||
        !localpath_set || !homepath_set || !snapshotfile_set) {
        LOG_ERROR("Must specify all arguments");
        cout << "Must specify all arguments" << endl;
        usage(argv[0]);
        exit(1);
    }

    if (Env::GetRank() == 0) {
        Env::InitDirs();
    }
    MPI_Barrier(MPI_COMM_WORLD);	// other process wait for shared dirs to be created
    Env::SetLogger();
    Env::LoadSampleTraceList(snapshot_file);
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
    //if (argc != 7) {
    //    usage(argv[0]);
    //    return 0;
    //}

    TimerPool::Start("Total");
    init(argc, argv);
    // cannot allocate 2d array because MPI only accepts one continous buffer
    char* send_buf  = new char[Env::GetNumTasks() * Env::GetMpiBufSize()];
    char* recv_buf  = new char[Env::GetNumTasks() * Env::GetMpiBufSize()];
    Env::SetSendBuf(send_buf);
    Env::SetRecvBuf(recv_buf);

    // avoid too many concurrent access to lustre
    sleep(1 * Env::GetRank());

    stringstream ss;
    ss << "Preparing traces:";
    LOG_INFO("preparing traces");
    TimerPool::Start("PrepareTrace");
    // local-1: prepare traces, partition indices
    int i = 0;
    while (Env::LvmidToVmid(i) >= 0) { //we use this mapping function because it returns all vms for this machine, not just those for the current round, and we want to prepare all traces at the start
        int vmid = Env::LvmidToVmid(i);
        ss << " " << vmid;
        int ssid = Env::GetNumSnapshots();
        string source_trace = Env::GetSampleTrace(vmid);
        string mixed_trace = Env::GetVmTrace(vmid);
        SnapshotMixer mixer(vmid, ssid, source_trace, mixed_trace);
        mixer.Generate();
        i++;
    }
    TimerPool::Stop("PrepareTrace");    
    LOG_INFO(ss.str());

    LOG_INFO("loading partition index from lustre to local directory");
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

    MPI_Barrier(MPI_COMM_WORLD);	// sync the progress before taking measurements

    TimerPool::Start("Deduplication");

    int round = 0;
    while (Env::InitRound(round)) {
        round++;
        LOG_INFO("Starting Round " << round);
        TimerPool::Start("RoundTime");

        struct pstat begin, end;
        pid_t mypid = getpid();
        get_usage(mypid, &begin);

        LOG_INFO("exchanging dirty blocks");
        TimerPool::Start("1aExchangeDirtyBlocks");
        // mpi-1: exchange dirty segments
        do {
            MpiEngine* p_mpi = new MpiEngine();
            TraceReader* p_reader = new TraceReader();
            RawRecordAccumulator* p_accu = new RawRecordAccumulator();

            p_mpi->SetDataSpout(dynamic_cast<DataSpout*>(p_reader));
            p_mpi->SetDataSink(dynamic_cast<DataSink*>(p_accu));
            p_mpi->SetTimerPrefix("ExchangeDirtyBlocks");
            p_mpi->Start();
            LOG_INFO("ExchangeDirtyBlocks Rounds of MPI Communication: " << p_mpi->GetMpiCount()); 
            p_reader->Stat();
            p_accu->Stat();

            delete p_mpi;
            delete p_reader;
            delete p_accu;
        } while(0);
        TimerPool::Stop("1aExchangeDirtyBlocks");

        LOG_INFO("making dedup comparison");
        uint64_t indexEntries = 0;
        
        uint64_t s2dedup_input_count = 0;
        uint64_t s2dup_output_count = 0;
        uint64_t s2dupnew_output_count = 0;
        uint64_t s2new_output_count = 0;
        TimerPool::Start("1bDedupComparison");
        // local-2: compare with partition index
        for (int partid = Env::GetPartitionBegin(); partid < Env::GetPartitionEnd(); partid++) {
            PartitionIndex index;
            index.FromFile(Env::GetLocalIndexName(partid));
            indexEntries += (uint64_t)index.getNumEntries();
            RecordReader<Block> reader(Env::GetStep2InputName(partid));
            Block blk;
            BlockMeta bm;
            DedupBuffer buf;
            RecordWriter<BlockMeta> output1(Env::GetStep2OutputDupBlocksName(partid));
            RecordWriter<Block> output2(Env::GetStep2OutputDupWithNewName(partid));
            RecordWriter<Block> output3(Env::GetStep2OutputNewBlocksName(partid));
            //int partitionDups = 0;
            //int partitionDupNew = 0;
            //int partitionNew = 0;

            while(reader.Get(blk)) {
                s2dedup_input_count++;
                // dup with existing blocks?
                if (index.Find(blk.mCksum)) {
                    s2dup_output_count++;
                    //partitionDups++;
                    bm.mBlk = blk;
                    bm.mRef = REF_VALID;
                    output1.Put(bm);
                    continue;
                }
                // dup with new blocks in this run?
                if (buf.Find(blk)) {
                    s2dupnew_output_count++;
                    //partitionDupNew++;
                    output2.Put(blk);
                    continue;
                }
                // completely new
                //partitionNew++;
                s2new_output_count++;
                buf.Add(blk);
                output3.Put(blk);
            }
            //LOG_INFO("Step 2 Partition " << partid << " Summary\tnew: " << partitionNew << "\tdupnew: " << partitionDupNew << "\tdup: " << partitionDups);
        }    

        TimerPool::Stop("1bDedupComparison");
        LOG_INFO("Step 2 Summary: blocks read: " << s2dedup_input_count);
        LOG_INFO("Step 2 Summary: dup blocks written: " << s2dup_output_count);
        LOG_INFO("Step 2 Summary: dup-with-new blocks written: " << s2dupnew_output_count);
        LOG_INFO("Step 2 Summary: new blocks written: " << s2new_output_count);
        LOG_INFO("Step 2 Summary: total blocks written: " << (s2new_output_count + s2dupnew_output_count + s2dup_output_count));
        LOG_INFO("Total entries in current partitions: " << indexEntries);
        Env::StatPartitionIndexSize();

        LOG_INFO("exchange new blocks");
        TimerPool::Start("2aExchangeNewBlocks");
        // mpi-2: exchange new blocks
        do {
            MpiEngine* p_mpi = new MpiEngine();
            NewBlockReader* p_reader = new NewBlockReader();
            NewRecordAccumulator* p_accu = new NewRecordAccumulator();

            p_mpi->SetDataSpout(dynamic_cast<DataSpout*>(p_reader));
            p_mpi->SetDataSink(dynamic_cast<DataSink*>(p_accu));
            p_mpi->SetTimerPrefix("ExchangeNewBlocks");
            p_mpi->Start();
            LOG_INFO("ExchangeNewBlocks Rounds of MPI Communication: " << p_mpi->GetMpiCount()); 
            p_reader->Stat();
            p_accu->Stat();

            delete p_mpi;
            delete p_reader;
            delete p_accu;
        } while (0);
        TimerPool::Stop("2aExchangeNewBlocks");    

        LOG_INFO("writing new blocks to backup storage");
        uint64_t s3block_count = 0;
        TimerPool::Start("2bWriteNewBlocks");
        // local-3: write new blocks to storage
        for (i = 0; Env::GetVmId(i) >= 0; i++) {
            int vmid = Env::GetVmId(i);
            RecordReader<Block> input(Env::GetStep3InputName(vmid));
            RecordWriter<BlockMeta> output(Env::GetStep3OutputName(vmid));
            Block blk;
            BlockMeta bm;
            while (input.Get(blk)) {
                s3block_count++;
                bm.mBlk = blk;
                bm.mRef = REF_VALID;
                output.Put(bm);
            }
        }
        TimerPool::Stop("2bWriteNewBlocks");
        LOG_INFO("Step 3 Summary: blocks read and written: " << s3block_count);

        LOG_INFO("exchanging data reference of new blocks");
        TimerPool::Start("3aExchangeNewRefs");
        // mpi-3: exchange new block ref
        do {
            MpiEngine* p_mpi = new MpiEngine();
            NewRefReader* p_reader = new NewRefReader();
            NewRefAccumulator* p_accu = new NewRefAccumulator();

            p_mpi->SetDataSpout(dynamic_cast<DataSpout*>(p_reader));
            p_mpi->SetDataSink(dynamic_cast<DataSink*>(p_accu));
            p_mpi->SetTimerPrefix("ExchangeNewRefs");
            p_mpi->Start();
            LOG_INFO("ExchangeNewRefs Rounds of MPI Communication: " << p_mpi->GetMpiCount()); 
            p_reader->Stat();
            p_accu->Stat();

            delete p_mpi;
            delete p_reader;
            delete p_accu;
        } while (0);
        TimerPool::Stop("3aExchangeNewRefs");

        LOG_INFO("updating partition index and dup_new block references");
        uint64_t dupNewBlocks = 0;
        TimerPool::Start("3bUpdateRefAndIndex");
        // local-4: update refs to pending blocks, then update partition index
        for (int partid = Env::GetPartitionBegin(); partid < Env::GetPartitionEnd(); partid++) {
            PartitionIndex index;
            index.FromFile(Env::GetStep4InputName(partid));
            RecordReader<Block> input(Env::GetStep2OutputDupWithNewName(partid));
            RecordWriter<BlockMeta> output(Env::GetStep2OutputDupBlocksName(partid), true);
            Block blk;
            BlockMeta bm;
            while (input.Get(blk)) {
                dupNewBlocks++;
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
            //input.Stat();
            //output.Stat();
        }
        TimerPool::Stop("3bUpdateRefAndIndex");
        LOG_INFO("Reference Update Summary: dup-with-new blocks: " << dupNewBlocks);

        LOG_INFO("exchanging dup blocks");
        TimerPool::Start("4ExchangeDupBlocks");
        // mpi-4: exchange dup block meta
        do {
            MpiEngine* p_mpi = new MpiEngine();
            DupBlockReader* p_reader = new DupBlockReader();
            DupRecordAccumulator* p_accu = new DupRecordAccumulator();

            p_mpi->SetDataSpout(dynamic_cast<DataSpout*>(p_reader));
            p_mpi->SetDataSink(dynamic_cast<DataSink*>(p_accu));
            p_mpi->SetTimerPrefix("ExchangeDupBlocks");
            p_mpi->Start();
            LOG_INFO("ExchangeDupBlocks Rounds of MPI Communication: " << p_mpi->GetMpiCount()); 
            p_reader->Stat();
            p_accu->Stat();

            delete p_mpi;
            delete p_reader;
            delete p_accu;
        } while (0);
        TimerPool::Stop("4ExchangeDupBlocks");

        get_usage(mypid, &end);
        double user_usage, system_usage;
        calc_cpu_usage_pct(&end, &begin, &user_usage, &system_usage);
        LOG_INFO("cpu usage %: user " << user_usage << ", system " << system_usage);

        sleep(1 * Env::GetRank());
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
        //
        //the stop is implicit
        //TimerPool::Stop("Deduplication");
        //TimerPool::Stop("RoundTime");

        TimerPool::PrintAll();
        TimerPool::Start("Total"); //print stops the timer, so we must restart
        TimerPool::Start("Deduplication");
        TimerPool::Reset("RoundTime"); //we want round time to be per-round

    }
    // clean up
        //TimerPool::Stop("Total"); //implicit in print
        //TimerPool::PrintAll();
    delete[] send_buf;
    delete[] recv_buf;
    final();
    MPI_Finalize();
    return 0;
}


















