#ifndef _DATA_SINK_H_
#define _DATA_SINK_H_

#include <fstream>
#include "trace_types.h"
#include "disk_io.h"
#include <map>
#include "partition_index.h"

using namespace std;

// base class for data receiver
class DataSink
{
public:
    DataSink();

    virtual ~DataSink();

    virtual void ProcessBuffer() = 0;
    virtual void Stat() = 0;

public:
    DataRecord* mRecord;

protected:
    MsgHeader   mHeader;
    int         mReadPos;       // reading position of current buffer
    char*       mRecvBuf;       // current receive buffer
    char*       mEndOfBuf;      // end of the last receive buffer
    int         mBufSize;
    int         mRecordSize;

protected:
    // after data processing, must be restored to initial state
    void Reset();

    // extract the next record from receive buffers, store in mRecord,
    // return true on success, false if no record can be extracted.
    bool GetRecord();
};

// save raw records into q partitions
class RawRecordAccumulator : public DataSink
{
public:
    RawRecordAccumulator();
    ~RawRecordAccumulator();

    void ProcessBuffer();
    void Stat();


private:
    RecordWriter<Block>** mWriterPtrs;
    uint64_t mStatRecordCount;
};

// save new blocks by vm
class NewRecordAccumulator : public DataSink
{
public:
    NewRecordAccumulator();
    ~NewRecordAccumulator();

    void ProcessBuffer();
    void Stat();

private:
    map<int, RecordWriter<Block>*> mWriters;
    uint64_t mStatRecordCount;
};


// save new refs by partition
class NewRefAccumulator : public DataSink
{
public:
    NewRefAccumulator();
    ~NewRefAccumulator();

    void ProcessBuffer();
    void Stat();

private:
    RecordWriter<IndexEntry>** mWriterPtrs;
    uint64_t mStatRecordCount;
};

// save dup block meta by vm
class DupRecordAccumulator : public DataSink
{
public:
    DupRecordAccumulator();
    ~DupRecordAccumulator();

    void ProcessBuffer();
    void Stat();

private:
    map<int, RecordWriter<BlockMeta>*> mWriters;
    uint64_t mStatRecordCount;
};


#endif

















