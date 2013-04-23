#include "data_sink.h"
#include "batch_dedup_config.h"
#include "mpi_engine.h"

DataSink::DataSink()
{
    mRecvBuf = Env::GetRecvBuf();
    mReadPos = 0;
    mEndOfBuf = mRecvBuf + Env::GetMpiBufSize() * Env::GetNumTasks();
    mBufSize = Env::GetMpiBufSize();
}

DataSink::~DataSink()
{
}

bool DataSink::GetRecord()
{
    while ((mRecvBuf + mReadPos) >= mEndOfBuf) {
        // initialize header if at the beginning of a buffer
        if (mReadPos == 0) {
            mHeader.FromBuffer(&mRecvBuf[0]);
            mReadPos += mHeader.GetSize();
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
    int num_parts = Env::GetNumPartitions();
    mBuffers = new char*[num_parts];
    mOutputs = new ofstream[num_parts];
    for (int i = 0; i < num_parts; i++) {
        mBuffers[i] = new char[Env::GetWriteBufSize()];
        mOutputs[i].rdbuf()->pubsetbuf(mBuffers[i], Env::GetWriteBufSize());
    }
}

RawRecordAccumulator::~RawRecordAccumulator()
{
    int num_tasks = Env::GetNumTasks();
    for (int i = 0; i < num_tasks; i++) {
        mOutputs[i].close();
        delete[] mBuffers[i];
    }
    delete[] mBuffers;
    delete[] mOutputs;
}

void RawRecordAccumulator::ProcessBuffer()
{
    int num_parts = Env::GetNumPartitions();
    while (GetRecord()) {
        Block* pblk = static_cast<Block*>(mRecord);
        int part_id = pblk->mCksum.Last4Bytes() % num_parts;
        pblk->ToStream(mOutputs[part_id]);
    }
}

















