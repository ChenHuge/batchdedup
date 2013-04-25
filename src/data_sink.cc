#include "data_sink.h"
#include "batch_dedup_config.h"
#include "mpi_engine.h"

DataSink::DataSink()
{
    Reset();
}

DataSink::~DataSink()
{
}

void DataSink::Reset()
{
    mRecvBuf = Env::GetRecvBuf();
    mReadPos = 0;
    mEndOfBuf = mRecvBuf + Env::GetMpiBufSize() * Env::GetNumTasks();
    mBufSize = Env::GetMpiBufSize();
}

bool DataSink::GetRecord()
{
    while ((mRecvBuf + mReadPos) < mEndOfBuf) {
        // initialize header if at the beginning of a buffer
        if (mReadPos == 0) {
            mHeader.FromBuffer(&mRecvBuf[0]);
            mReadPos += mHeader.GetSize();
            LOG_DEBUG("process buffer: " << mHeader.ToString());
        }
        // go to the next buffer if current buffer no longer has data
        if ((mReadPos + mRecordSize) > mHeader.mTotalSize || mReadPos >= mBufSize) {
            mRecvBuf += mBufSize;
            mReadPos = 0;
            continue;
        }
        // read
        mRecord->FromBuffer(&mRecvBuf[mReadPos]);
        mReadPos += mRecordSize;
        return true;
    }
    return false;
}

RawRecordAccumulator::RawRecordAccumulator()
{
    mRecord = static_cast<DataRecord*>(new Block);
    mRecordSize = mRecord->GetSize();
    int num_parts = Env::GetNumPartitionsPerNode();
    mWriterPtrs = new RecordWriter<Block>*[num_parts];
    for (int partid = Env::GetPartitionBegin(); partid < Env::GetPartitionEnd(); ++partid)
    {
        mWriterPtrs[partid % num_parts] = 
            new RecordWriter<Block>(Env::GetStep2InputName(partid));
    }
}

RawRecordAccumulator::~RawRecordAccumulator()
{
    int num_parts = Env::GetNumPartitionsPerNode();
    for (int partid = Env::GetPartitionBegin(); partid < Env::GetPartitionEnd(); ++partid)
    {
        delete mWriterPtrs[partid % num_parts];
    }
    delete[] mWriterPtrs;
}

void RawRecordAccumulator::ProcessBuffer()
{
    int num_parts = Env::GetNumPartitionsPerNode();
    int num_records = 0;
    while (GetRecord()) {
        Block* pblk = static_cast<Block*>(mRecord);
        int part_id = Env::GetPartitionId(pblk->mCksum);
        mWriterPtrs[part_id % num_parts]->Put(*pblk);
        num_records++;
    }
    LOG_DEBUG("processed " << num_records << " records");
    Reset();
}

















