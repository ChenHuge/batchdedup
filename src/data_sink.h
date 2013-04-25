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

private:
    RecordWriter<Block>** mWriterPtrs;
};

// save new blocks by vm
class NewRecordAccumulator : public DataSink
{
public:
    NewRecordAccumulator();
    ~NewRecordAccumulator();

    void ProcessBuffer();

private:
    map<int, RecordWriter<Block>*> mWriters;
};


// save new refs by partition
class NewRefAccumulator : public DataSink
{
public:
    NewRefAccumulator();
    ~NewRefAccumulator();

    void ProcessBuffer();

private:
    RecordWriter<IndexEntry>** mWriterPtrs;
};

#endif
















