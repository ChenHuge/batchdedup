#ifndef _DATA_SPOUT_H_
#define _DATA_SPOUT_H_

#include <fstream>
#include "trace_types.h"
#include "disk_io.h"
#include "partition_index.h"
#include "snapshot_meta.h"

using namespace std;

class DataSpout
{
public:
    virtual ~DataSpout();

    virtual bool GetRecord(DataRecord*& pdata) = 0;

    virtual int GetRecordDest(DataRecord* pdata) = 0;

    virtual int GetRecordSize() = 0;

    virtual void Stat() = 0;
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

    // override
    void Stat();

private:
    int mNumTasks;
    ifstream mInput;
    char* mReadBuf;
    int mVmIdx;
    uint64_t mStatTotalSize;
    uint64_t mStatDirtySize;
    uint64_t mStatTotalCount;
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

    // override
    void Stat();

private:
    RecordReader<Block>* mInputPtr;
    uint64_t mStatNewCount;
    int mPartId;
};

class NewRefReader : public DataSpout
{
public:
    IndexEntry mRecord;

public:
    NewRefReader();

    // override
    ~NewRefReader();

    // override
    bool GetRecord(DataRecord*& pdata);

    // override
    int GetRecordDest(DataRecord* pdata);

    // override
    int GetRecordSize();

    // override
    void Stat();

private:
    RecordReader<BlockMeta>* mInputPtr;
    int mVmIdx;
    uint64_t mStatNewSize;
    uint64_t mStatNewCount;
};

class DupBlockReader : public DataSpout
{
public:
    BlockMeta mRecord;

public:
    DupBlockReader();

    // override
    ~DupBlockReader();

    // override
    bool GetRecord(DataRecord*& pdata);

    // override
    int GetRecordDest(DataRecord* pdata);

    // override
    int GetRecordSize();

    // override
    void Stat();

private:
    RecordReader<BlockMeta>* mInputPtr;
    int mPartId;
    uint64_t mStatDupCount;
};

#endif
