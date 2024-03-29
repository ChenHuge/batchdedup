#include "mpi_engine.h"
#include "batch_dedup_config.h"
#include "mpi.h"
#include "timer.h"

MpiEngine::MpiEngine()
	: mSpoutPtr(NULL),
      mSinkPtr(NULL),
      mWritePos(NULL),
      mDispls(NULL),
      mHeaders(NULL)
{
    mWritePos = new int[Env::GetNumTasks()];
    mDispls = new int[Env::GetNumTasks()];
    mRecvCounts = new int[Env::GetNumTasks()];
    mHeaders = new MsgHeader[Env::GetNumTasks()];
    mpiCounter = 0;
}

MpiEngine::~MpiEngine()
{
    if (mWritePos != NULL)
        delete[] mWritePos;
    if (mDispls != NULL)
        delete[] mDispls;
    if (mHeaders != NULL)
        delete[] mHeaders;
}

void MpiEngine::Init()
{
    int sz = mHeaders[0].GetSize();
    for (int i = 0; i < Env::GetNumTasks(); i++) {
        mWritePos[i] = sz;
        mHeaders[i].mRecordSize = (uint16_t)mSpoutPtr->GetRecordSize();
        mDispls[i] = i * Env::GetMpiBufSize();
        mRecvCounts[i] = 0;
    }
}

void MpiEngine::Start()
{
    Init();
    DataRecord* pd;
    int header_size = mHeaders[0].GetSize();
    char* send_buf = Env::GetSendBuf();
    char* recv_buf = Env::GetRecvBuf();
    size_t buf_size = Env::GetMpiBufSize();
    int num_tasks = Env::GetNumTasks();
    int rc;
    string read_timer = timerPrefix + ":MpiRead";
    string mpi_timer = timerPrefix + ":MpiNetwork";
    string processing_timer = timerPrefix + ":MpiProcessing";

    while(true) {
        // fill send buffer if we have data to read
        TimerPool::Start(read_timer);
        if (mSpoutPtr->GetRecord(pd)) {
            int dest = mSpoutPtr->GetRecordDest(pd);
            pd->ToBuffer(&send_buf[(dest * buf_size) + mWritePos[dest]]);
            mWritePos[dest] += pd->GetSize();
            mHeaders[dest].mNumRecords += 1;
            if (mWritePos[dest] < (buf_size - pd->GetSize()))
                continue;	// buffer nut full, read more
        }
        TimerPool::Stop(read_timer);

        // buffer full or cannot read more, need to send
        // prepare header
        bool has_data = false;
        for (int i = 0; i < num_tasks; ++i) {
            if (mWritePos[i] > header_size) {
                has_data = true;
            }
        }
        uint16_t send_flag = has_data ? SENDER_HAS_DATA : SENDER_HAS_NO_DATA;
        for (int i = 0; i < num_tasks; ++i) {
            mHeaders[i].mTotalSize = mWritePos[i];
            mHeaders[i].mFlags = send_flag;
            mHeaders[i].ToBuffer(&send_buf[i * buf_size]);
        }
        TimerPool::Start(mpi_timer);
        // send recv size
        LOG_DEBUG("sending msg size");
        rc = MPI_Alltoall(mWritePos, 1, MPI_INT, 
                          mRecvCounts, 1, MPI_INT, 
                          MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) {
            LOG_ERROR("send rcount fail");
            break;
        }

        // send data
        LOG_DEBUG("sending data");
        rc = MPI_Alltoallv(send_buf, mWritePos, mDispls, MPI_CHAR, 
                           recv_buf, mRecvCounts, mDispls, MPI_CHAR, 
                           MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) {
            LOG_ERROR("send data fail");
            break;
        }
        TimerPool::Stop(mpi_timer);
        mpiCounter++;
        
        // check finish condition
        bool finished = true;
        MsgHeader hd;
        for (int i = 0; i < num_tasks; i++) {
            mHeaders[i].mNumRecords = 0;	// also clear header state here
            hd.FromBuffer(&recv_buf[i * buf_size]);
            if (hd.mFlags == SENDER_HAS_DATA)
                finished = false;
        }

        // process data
        TimerPool::Start(processing_timer);
        mSinkPtr->ProcessBuffer();
        
        // reset send buffer
        for (int i = 0; i < num_tasks; i++) {
           mWritePos[i] = header_size;
        }
        TimerPool::Stop(processing_timer);

        if (finished) {
            LOG_DEBUG("nobody has data to send, stop");
            break;
        }
    }
}

void MpiEngine::SetDataSpout(DataSpout* spout)
{
    mSpoutPtr = spout;
}

void MpiEngine::SetDataSink(DataSink* sink)
{
    mSinkPtr = sink;
}
void MpiEngine::SetTimerPrefix(string prefix)
{
    timerPrefix = prefix;
}
int MpiEngine::GetMpiCount()
{
    return mpiCounter;
}
