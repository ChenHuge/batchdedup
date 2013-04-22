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

private:
    DataSpout* mSpoutPtr;
    DataSink*  mSinkPtr;
    int*       mWritePos;
    int*       mDispls;
    int*       mRecvCounts;
    MsgHeader* mHeaders;

private:
    void Init();
};

#endif















