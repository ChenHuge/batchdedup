#ifndef _DATA_SPOUT_H_
#define _DATA_SPOUT_H_

#include <fstream>
#include "trace_types.h"
#include "disk_io.h"

using namespace std;

class DataSpout
{
public:
    virtual ~DataSpout();

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

    // override
    ~TraceReader();

    // override
    bool GetRecord(DataRecord*& pdata);

    // override
    int GetRecordDest(DataRecord* pdata);

    // override
    int GetRecordSize();

private:
    int mNumTasks;
    ifstream mInput;
    char* mReadBuf;
    int mVmIdx;
};

class NewBlockReader : public DataSpout
{
public:
    Block mBlk;

public:
    NewBlockReader();

    // override
    ~NewBlockReader();

    // override
    bool GetRecord(DataRecord*& pdata);

    // override
    int GetRecordDest(DataRecord* pdata);

    // override
    int GetRecordSize();

private:
    RecordReader<Block>* mInputPtr;
    int mPartId;
};

#endif
