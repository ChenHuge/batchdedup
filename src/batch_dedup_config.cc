#include "batch_dedup_config.h"
#include "trace_types.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

const double   VM_SEG_CHANGE_RATE   = 0.5;
const double   VM_BLOCK_CHANGE_RATE = 0.5;
const double   SS_SEG_CHANGE_RATE   = 0.22;
const double   SS_BLOCK_CHANGE_RATE = 0.255;
const int      MAX_NUM_SNAPSHOTS    = 100;
const uint16_t BLOCK_CLEAN_FLAG     = 0;
const uint16_t BLOCK_DIRTY_FLAG     = 1;

int            Env::mRound = -1;
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
//vector<int>    Env::mMyVmIds; // a list of VM IDs that belong to this MPI instance for this backup round
vector<vector<int> > Env::mMyVmSchedule; //list of VM IDs for this instance to backup in each round
string         Env::mLocalPath; // path to local storage
string         Env::mRemotePath; // path to remote storage (lustre)
string         Env::mHomePath; // path to home directory
ofstream       Env::mLogger;
size_t         Env::mIndexSize = 0;

int Env::VmidToMid(int vmid) {
    if (vmid > GetNumVms()) {
        return -1;
    } else {
        return vmid % mNumTasks;
    }
}

int Env::LvmidToVmid(int vmid) {
    if (vmid > (mNumVms / mNumTasks)) {
        return -1;
    } else {
        return mRank + (mNumTasks * vmid);
    }
}

int Env::VmidToSid(int vmid) {
    if (vmid > GetNumVms()) {
        return -1;
    } else {
        return vmid % mSampleTraces.size();
    }
}

void Env::ScheduleVMs(RoundScheduler* scheduler)
{
    size_t num_samples = mSampleTraces.size();
    vector<double> samplesizes;
    
    for(int i = 0; i < num_samples; i++) {
        string sample_path = GetSampleTrace(i);
        struct stat filestatus;
        stat( sample_path.c_str(), &filestatus );
        //36 bytes per chunk entry, avg chunk size 4.5KB
        samplesizes.push_back((filestatus.st_size/36)*1024*4.5);
    }
    vector<vector<double> > loads;
    int vmid = 0;
    for (int i = 0; i < mNumVms; i++) {
        while (loads.size() <= VmidToMid(i)) {
            vector<double> empty_machine;
            loads.push_back(empty_machine);
        }
        //cerr << ">> loads[" << (VmidToMid(i)) << "].add " << samplesizes[VmidToSid(i)] << endl;
        loads[VmidToMid(i)].push_back(samplesizes[VmidToSid(i)]);
    }
    //first construct vm/machine loads (we need to estimate machine size for this)
    //then run scheduler until finished,
    //adding this machine's schedule for each round
    scheduler->setMachineList(loads);
    scheduler->setTimeLimit(60*60*3);
    vector<vector<int> > round_schedule;
    while (scheduler->schedule_round(round_schedule)) {
        //cout << "adding round:";
        //for(int i = 0; i < round_schedule[mRank].size(); i++) {
        //    cout << " " << round_schedule[mRank][i];
        //}
        //cout << endl;
        for(int i = 0; i < round_schedule[mRank].size(); i++) {
            round_schedule[mRank][i] = LvmidToVmid(round_schedule[mRank][i]);
        }
        mMyVmSchedule.push_back(round_schedule[mRank]);
    }

}

void Env::SetRank(int rank) 
{ 
    Env::mRank = rank; 
}

int Env::GetRank() 
{ 
    return mRank; 
}

bool Env::InitRound(int r)
{
    mRound = r;
    if (mMyVmSchedule.size() == 0) {
        NullScheduler scheduler;
        Env::ScheduleVMs(&scheduler);
    }
    return mMyVmSchedule.size() > r;
    //mMyVmIds.clear(); //so it will be rebuilt on the next call to GetVmId
}

int Env::GetRound()
{
    return mRound;
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
        if(trace_fname.length() < 1 || trace_fname.c_str()[0] == '\n') {
            continue;
        }
        mSampleTraces.push_back(trace_fname);
        //LOG_INFO("Added trace file: " << trace_fname);
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
    //if (mMyVmIds.size() == 0) {
    //    for (int i = 0; i < mNumVms; i++) {
    //        if (i % mNumTasks == mRank) {
    //            mMyVmIds.push_back(i);
    //        }
    //    }
    //}
    //
    //if (idx < mMyVmIds.size()) {
    //    return mMyVmIds[idx];
    //}
    //else {
    //    return -1;
    //}

    if (mMyVmSchedule.size() == 0) {
        NullScheduler scheduler;
        Env::ScheduleVMs(&scheduler);
    }

    if (mRound < mMyVmSchedule.size() && idx < mMyVmSchedule[mRound].size()) {
        cout << "Machine " << mRank << " round[" << mRound << "," << idx << "] = " << mMyVmSchedule[mRound][idx] << endl;
        return mMyVmSchedule[mRound][idx];
    }
    else {
        //cout << "Machine " << mRank << " round " << mRound << " has no vm " << idx << endl;
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
    CreateDir(mRemotePath);
    CreateDir(mHomePath);
    for (uint16_t i = 0; i <= 0xFF; i++) {
        stringstream ss;
        ss << mRemotePath << i;
        CreateDir(ss.str().c_str());
    }
}

void Env::CreateDir(string const &path, bool empty)
{
    if (FileExists(path)) {
        if (empty) {
            stringstream ss;
            ss << "rm -rf " << path << "/*";
            system(ss.str().c_str());
        }
        return;
    }
    else {
        stringstream ss;
        ss << "mkdir " << path;
        system(ss.str().c_str());
    }
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

string Env::GetStep2OutputDupBlocksName(int partid)
{
    stringstream ss;
    ss << mLocalPath << "partition." << partid << ".step2.out1";
    return ss.str();
}

string Env::GetStep2OutputDupWithNewName(int partid)
{
    stringstream ss;
    ss << mLocalPath << "partition." << partid << ".step2.out2";
    return ss.str();
}

string Env::GetStep2OutputNewBlocksName(int partid)
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
    return cksum.Middle4Bytes() % mNumPartitions;
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
    int numVms = 0;
    for(int i = 0; i < mMyVmSchedule.size(); i++) {
        numVms += mMyVmSchedule[i].size();
    }
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
       << ", Vms on this node " << numVms //mMyVmIds.size()
       << ", partitions per node " << GetNumPartitionsPerNode()
       << ", begin of partition " << GetPartitionBegin()
       << ", end of partition " << GetPartitionEnd()
        ;
    return ss.str();
}

void Env::AddPartitionIndexSize(size_t len)
{
    mIndexSize += len;
}

void Env::StatPartitionIndexSize()
{
    LOG_INFO("average partition index size: " << (mIndexSize / GetNumPartitionsPerNode()));
}

bool Env::FileExists(const string& fname)
{
    struct stat st;
    if (stat(fname.c_str(), &st) == -1 && errno == ENOENT) {
        return false;
    }
    else {
        return true;
    }
}

















