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

using namespace std;

void usage(char* progname)
{
    cout << "Usage: " << progname << " num_partitions num_VMs num_snapshots mpi_buffer read_buffer write_buffer" << endl;
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
    Env::SetRemotePath("/oasis/triton/scratch/wei-ucsb/");
    Env::SetLocalPath("/state/partition1/batchdedup/");
    Env::SetHomePath("/home/wei-ucsb/batchdedup/");
    Env::SetLogger();
    Env::LoadSampleTraceList("/home/wei-ucsb/batchdedup/sample_traces");
    LOG_INFO(Env::ToString());
}

void final()
{
    stringstream ss;
    ss << "cp -r " << Env::GetLocalPath() << " " << Env::GetRemotePath();
    system(ss.str().c_str());
}

int main(int argc, char** argv)
{
    if (argc != 7) {
        usage(argv[0]);
        return 0;
    }

    init(argc, argv);
    // cannot allocate 2d array because MPI only accepts one continous buffer
    char* send_buf  = new char[Env::GetNumTasks() * Env::GetMpiBufSize()];
    char* recv_buf  = new char[Env::GetNumTasks() * Env::GetMpiBufSize()];
    Env::SetSendBuf(send_buf);
    Env::SetRecvBuf(recv_buf);

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
    for (i = Env::GetPartitionBegin(); i < Env::GetPartitionEnd(); i++) {
        string remote_fname = Env::GetRemoteIndexName(i);
        string local_fname = Env::GetLocalIndexName(i);
        stringstream cmd;
        cmd << "cp " << remote_fname << " " << local_fname;
        system(cmd.str().c_str());
    }
    
    // mpi-1: exchange dirty segments
    MpiEngine* p_step1 = new MpiEngine();
    TraceReader* p_reader = new TraceReader();
    RawRecordAccumulator* p_accu = new RawRecordAccumulator();

    p_step1->SetDataSpout(dynamic_cast<DataSpout*>(p_reader));
    p_step1->SetDataSink(dynamic_cast<DataSink*>(p_accu));
    p_step1->Start();

    delete p_step1;
    delete p_reader;
    delete p_accu;
    
    delete[] send_buf;
    delete[] recv_buf;
    MPI_Finalize();
    Env::RemoveLocalPath();
    Env::CloseLogger();
    return 0;
}


















