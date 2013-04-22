#ifndef _DATA_SPOUT_H_
#define _DATA_SPOUT_H_

#include <fstream>
#include "trace_types.h"

using namespace std;

class DataSpout
{
public:
    virtual ~DataSpout() = 0;

    virtual bool GetRecord(DataRecord*& pdata) = 0;

    virtual int GetRecordDest(DataRecord* pdata) = 0;

    virtual int GetRecordSize() = 0;
};

class TraceReader : public DataSpout
{
public:
    Block mBlk;

public:
    TraceReader();

    ~TraceReader();

    bool GetRecord(DataRecord*& pdata);

    int GetRecordDest(DataRecord* pdata);

    int GetRecordSize();

private:
    int mNumTasks;
    ifstream mInput;
    char* mReadBuf;
    int mVmIdx;
};

#endif
