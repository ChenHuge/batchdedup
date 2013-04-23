#ifndef _BATCH_DEDUP_CONFIG_H_
#define _BATCH_DEDUP_CONFIG_H_

#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

// global constants
extern const double   VM_SEG_CHANGE_RATE;
extern const double   VM_BLOCK_CHANGE_RATE;
extern const double   SS_SEG_CHANGE_RATE;
extern const double   SS_BLOCK_CHANGE_RATE;
extern const int      MAX_NUM_SNAPSHOTS;
extern const uint16_t BLOCK_CLEAN_FLAG;
extern const uint16_t BLOCK_DIRTY_FLAG;

// global variables
class Env
{
public:
    static void SetRank(int rank);
    static int  GetRank();

    static void SetNumTasks(int num);
    static int  GetNumTasks();

    static void SetNumPartitions(int num);
    static int  GetNumPartitions();

    static void   SetMpiBufSize(size_t size);
    static size_t GetMpiBufSize();

    static void  SetSendBuf(char* buf);
    static char* GetSendBuf();

    static void  SetRecvBuf(char* buf);
    static char* GetRecvBuf();

    static void   SetReadBufSize(size_t size);
    static size_t GetReadBufSize();

    static void   SetWriteBufSize(size_t size);
    static size_t GetWriteBufSize();

    static void SetNumVms(int num);
    static int  GetNumVms();

    static void SetNumSnapshots(int num);
    static int  GetNumSnapshots();

    static string GetSampleTrace(int vmid);
    static void   LoadSampleTraceList(string fname);

    static string GetVmTrace(int vmid);

    static int GetVmId(size_t idx);

    static void SetLogger();
    static void CloseLogger();

    static void   SetLocalPath(const string& path);
    static string GetLocalPath();

    static void   SetRemotePath(const string& path);
    static string GetRemotePath();

    static void   SetHomePath(const string& path);
    static string GetHomePath();

    static string GetTaskName();

    static void   CreateDir(const string& path, bool empty = false);

public:
    static ofstream mLogger;

private:
    static int            mRank;
    static int            mNumTasks;
    static int            mNumPartitions;
    static int            mNumVms;
    static int            mNumSnapshots;
    static size_t         mMpiBufSize;
    static char*          mSendBuf;
    static char*          mRecvBuf;
    static size_t         mReadBufSize;
    static size_t         mWriteBufSize;
    static vector<string> mSampleTraces; // a list of sample trace files
    static vector<int>    mMyVmIds; // a list of VM IDs that belong to this MPI instance
    static string         mLocalPath; // path to local storage
    static string         mRemotePath; // path to remote storage (lustre)
    static string         mHomePath; // path to home directory
};

#define LOG_ERROR(msg)	\
    do {	\
    	Env::mLogger << "ERROR: " << __PRETTY_FUNCTION__<< msg << endl;	\
    } while (0)

#define LOG_INFO(msg)	\
    do {	\
    	Env::mLogger << "INFO: " << __PRETTY_FUNCTION__<< msg << endl;	\
    } while (0)

#ifdef DEBUG
#define LOG_DEBUG(msg)	\
    do {	\
    	Env::mLogger << "DEBUG: " << __PRETTY_FUNCTION__<< msg << endl;	\
    } while (0)
#else
#define LOG_DEBUG(msg) do {} while (0)
#endif

#endif                          // _BATCH_DEDUP_CONFIG_H_

















