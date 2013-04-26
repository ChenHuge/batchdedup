#include "batch_dedup_config.h"
#include "trace_types.h"
#include <fstream>
#include <sstream>

const double   VM_SEG_CHANGE_RATE   = 0.5;
const double   VM_BLOCK_CHANGE_RATE = 0.5;
const double   SS_SEG_CHANGE_RATE   = 0.2;
const double   SS_BLOCK_CHANGE_RATE = 0.5;
const int      MAX_NUM_SNAPSHOTS    = 100;
const uint16_t BLOCK_CLEAN_FLAG     = 0;
const uint16_t BLOCK_DIRTY_FLAG     = 1;

int            Env::mRank = -1;
int            Env::mNumTasks = -1;
int            Env::mNumPartitions = -1;
int            Env::mNumVms = -1;
int            Env::mNumSnapshots = -1;
size_t         Env::mMpiBufSize = 0;
char*          Env::mSendBuf = NULL;
char*          Env::mRecvBuf = NULL;
size_t         Env::mReadBufSize = 0;
size_t         Env::mWriteBufSize = 0;
vector<string> Env::mSampleTraces; // a list of sample trace files
vector<int>    Env::mMyVmIds; // a list of VM IDs that belong to this MPI instance
string         Env::mLocalPath; // path to local storage
string         Env::mRemotePath; // path to remote storage (lustre)
string         Env::mHomePath; // path to home directory
ofstream       Env::mLogger;

void Env::SetRank(int rank) 
{ 
    Env::mRank = rank; 
}

int Env::GetRank() 
{ 
    return mRank; 
}

void Env::SetNumTasks(int num) 
{ 
    mNumTasks = num; 
}

int Env::GetNumTasks() { 
    return mNumTasks; 
}

void Env::SetNumPartitions(int num) 
{ 
    mNumPartitions = num; 
}

int Env::GetNumPartitions() { 
    return mNumPartitions; 
}

void Env::SetMpiBufSize(size_t size) 
{ 
    mMpiBufSize = size; 
}

size_t Env::GetMpiBufSize() 
{ 
    return mMpiBufSize; 
}

void Env::SetReadBufSize(size_t size) 
{ 
    mReadBufSize = size; 
}

size_t Env::GetReadBufSize() 
{
    return mReadBufSize; 
}

void Env::SetWriteBufSize(size_t size) 
{ 
    mWriteBufSize = size; 
}

size_t Env::GetWriteBufSize() 
{
    return mWriteBufSize; 
}

void Env::SetNumVms(int num) 
{
    mNumVms = num; 
}

int Env::GetNumVms() 
{
    return mNumVms; 
}

void Env::SetNumSnapshots(int num) 
{
    mNumSnapshots = num; 
}

int Env::GetNumSnapshots() 
{
    return mNumSnapshots; 
}

string Env::GetSampleTrace(int vmid) 
{ 
    size_t num_samples = mSampleTraces.size();
    if (num_samples != 0) {
        return mSampleTraces[vmid % num_samples];
    }
    return "";
}

void Env::LoadSampleTraceList(string fname)
{
    ifstream is(fname.c_str(), ios::in);
    if (!is.is_open()) {
        LOG_ERROR("file not exist: " << fname);
        return;
    }

    string trace_fname("");
    while (is.good()) {
        getline(is, trace_fname);
        mSampleTraces.push_back(trace_fname);
        LOG_DEBUG("add trace file: " << trace_fname);
    }
    is.close();
}

string Env::GetVmTrace(int vmid)
{
    stringstream ss;
    ss << GetLocalPath() << "/vm." << vmid << "." << mNumSnapshots << ".trace";
    return ss.str();
}

int Env::GetVmId(size_t idx)
{
    if (mMyVmIds.size() == 0) {
        for (int i = 0; i < mNumVms; i++) {
            if (i % mNumTasks == mRank) {
                mMyVmIds.push_back(i);
            }
        }
    }

    if (idx < mMyVmIds.size()) {
        return mMyVmIds[idx];
    }
    else {
        return -1;
    }
}

void Env::SetLogger()
{
    stringstream ss;
    ss << mHomePath << mRank << ".log";
    mLogger.open(ss.str().c_str(), ios::out | ios::trunc);
    if (!mLogger.is_open()) {
        cerr << "unable to open log: " << ss.str();
    }
}

void Env::CloseLogger()
{
    if (mLogger.is_open()) {
        mLogger.close();
    }
}

void Env::SetLocalPath(string const &path)
{
    mLocalPath = path + "/" + Env::GetTaskName() + "/";
    CreateDir(path);
    CreateDir(mLocalPath, true);
}

string Env::GetLocalPath()
{
    return mLocalPath;
}

void Env::RemoveLocalPath()
{
    stringstream cmd;
    cmd << "rm -rf " << mLocalPath;
    system(cmd.str().c_str());
}

void Env::SetRemotePath(string const &path)
{
    stringstream ss;
    ss << path << "/" << mNumPartitions << "/";
    mRemotePath = ss.str();
}

string Env::GetRemotePath()
{
    return mRemotePath;
}

void Env::CopyToRemote(const string& fname)
{
    stringstream cmd;
    cmd << "cp " << fname << " " << mRemotePath;
    system(cmd.str().c_str());
}

void Env::SetHomePath(string const &path)
{
    mHomePath = path + "/" + Env::GetTaskName() + "/";
}

string Env::GetHomePath()
{
    return mHomePath;
}

string Env::GetTaskName()
{
    stringstream ss;
    ss << mNumTasks 
       << "_" << mNumPartitions
       << "_" << mNumVms 
       << "_" << mNumSnapshots 
       << "_" << mMpiBufSize 
       << "_" << mReadBufSize
       << "_" << mWriteBufSize;
    return ss.str();
}

void Env::InitDirs()
{
    CreateDir("/home/wei-ucsb/batchdedup/");
    CreateDir(mRemotePath);
    CreateDir(mHomePath);
}

void Env::CreateDir(string const &path, bool empty)
{
    if (empty) {
        stringstream ss;
        ss << "rm -rf " << path;
        system(ss.str().c_str());
    }
    
    // TODO: check dir existance before mkdir
    stringstream ss;
    ss << "mkdir " << path;
    system(ss.str().c_str());
}

void Env::SetSendBuf(char* buf)
{
    mSendBuf = buf;
}

char* Env::GetSendBuf()
{
    return mSendBuf;
}

void Env::SetRecvBuf(char *buf)
{
    mRecvBuf = buf;
}

char* Env::GetRecvBuf()
{
    return mRecvBuf;
}

int Env::GetNumPartitionsPerNode()
{
    if (mNumPartitions % mNumTasks == 0) {
        return mNumPartitions / mNumTasks;
    }
    else {
        return (mNumPartitions / mNumTasks) + 1;
    }
}

int Env::GetPartitionBegin()
{
    return mRank * GetNumPartitionsPerNode();
}

int Env::GetPartitionEnd()
{
    int end =  (mRank + 1) * GetNumPartitionsPerNode();
    if (end > mNumPartitions) {
        return mNumPartitions;
    }
    else {
        return end;
    }
}

string Env::GetRemoteIndexName(int partid)
{
    stringstream ss;
    int subdir = partid & 0xFF;
    ss << mRemotePath << subdir << "/" << "partition." << partid << ".index";
    return ss.str();
}

string Env::GetLocalIndexName(int partid)
{
    stringstream ss;
    ss << mLocalPath << "partition." << partid << ".index";
    return ss.str();
}

string Env::GetStep2InputName(int partid)
{
    stringstream ss;
    ss << mLocalPath << "partition." << partid << ".step2.in";
    return ss.str();
}

string Env::GetStep2Output1Name(int partid)
{
    stringstream ss;
    ss << mLocalPath << "partition." << partid << ".step2.out1";
    return ss.str();
}

string Env::GetStep2Output2Name(int partid)
{
    stringstream ss;
    ss << mLocalPath << "partition." << partid << ".step2.out2";
    return ss.str();
}

string Env::GetStep2Output3Name(int partid)
{
    stringstream ss;
    ss << mLocalPath << "partition." << partid << ".step2.out3";
    return ss.str();
}

string Env::GetStep3InputName(int vmid)
{
    stringstream ss;
    ss << mLocalPath << "vm." << vmid << ".step3.in";
    return ss.str();
}

string Env::GetStep3OutputName(int vmid)
{
    stringstream ss;
    ss << mLocalPath << "vm." << vmid << "." << mNumSnapshots << ".meta";
    return ss.str();
}

string Env::GetStep4InputName(int partid)
{
    stringstream ss;
    ss << mLocalPath << "partition." << partid << ".step4.in";
    return ss.str();
}

string Env::GetStep4OutputName(int vmid)
{
    stringstream ss;
    ss << mLocalPath << "vm." << vmid << "." << mNumSnapshots << ".step4.out";
    return ss.str();
}

int Env::GetPartitionId(const Checksum& cksum)
{
    return cksum.First4Bytes() % mNumPartitions;
}

int Env::GetDestNodeId(int partid)
{
    return partid / GetNumPartitionsPerNode();
}

int Env::GetDestNodeId(const Checksum& cksum)
{
    return GetDestNodeId(GetPartitionId(cksum));
}

int Env::GetSourceNodeId(int vmid)
{
    return vmid % mNumTasks;
}

string Env::ToString()
{
    stringstream ss;
    GetVmId(0);
    ss << "rank " << mRank
       << ", tasks " << mNumTasks
       << ", Vms " << mNumVms
       << ", snapshots " << mNumSnapshots
       << ", mpi buffer " << mMpiBufSize
       << ", read buffer " << mReadBufSize
       << ", write buffer " << mWriteBufSize
       << ", local path " << mLocalPath
       << ", remote path " << mRemotePath
       << ", home path " << mHomePath
       << ", Vms on this node " << mMyVmIds.size()
       << ", partitions per node " << GetNumPartitionsPerNode()
       << ", begin of partition " << GetPartitionBegin()
       << ", end of partition " << GetPartitionEnd()
        ;
    return ss.str();
}











