#ifndef _MPI_ENGING_H_
#define _MPI_ENGING_H_

#include "data_spout.h"
#include "data_sink.h"

class MpiEngine
{
public:
    MpiEngine();
    ~MpiEngine();

    void Start();
    void SetDataSpout(DataSpout* spout);
    void SetDataSink(DataSink* sink);
    void SetTimerPrefix(string prefix);
    int GetMpiCount();

private:
    DataSpout* mSpoutPtr;
    DataSink*  mSinkPtr;
    int*       mWritePos;
    int*       mDispls;
    int*       mRecvCounts;
    MsgHeader* mHeaders;
    string timerPrefix;
    uint64_t mpiCounter;

private:
    void Init();
};

#endif















