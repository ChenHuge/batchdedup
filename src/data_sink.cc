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


NewRecordAccumulator::NewRecordAccumulator()
{
    mRecord = static_cast<DataRecord*>(new Block);
    mRecordSize = mRecord->GetSize();
    for (int i = 0; Env::GetVmId(i) >= 0; i++) {
        int vmid = Env::GetVmId(i);
        mWriters[vmid] = new RecordWriter<Block>(Env::GetStep3InputName(vmid));
    }
}

NewRecordAccumulator::~NewRecordAccumulator()
{
    map<int, RecordWriter<Block>*>::iterator it;
    for (it = mWriters.begin(); it != mWriters.end(); it++) {
        delete it->second;
    }
}

void NewRecordAccumulator::ProcessBuffer()
{
    int num_records = 0;
    map<int, RecordWriter<Block>*>::iterator it;

    while (GetRecord()) {
        Block* pblk = static_cast<Block*>(mRecord);
        it = mWriters.find(pblk->mFileID);
        if (it == mWriters.end()) {
            LOG_ERROR("this block does not belong to current node " << pblk->mFileID);
        }
        else {
            it->second->Put(*pblk);
            num_records++;
        }
    }
    LOG_DEBUG("processed " << num_records << " records");
    Reset();
}


NewRefAccumulator::NewRefAccumulator()
{
    mRecord = static_cast<DataRecord*>(new IndexEntry);
    mRecordSize = mRecord->GetSize();
    int num_parts = Env::GetNumPartitionsPerNode();
    mWriterPtrs = new RecordWriter<IndexEntry>*[num_parts];
    for (int partid = Env::GetPartitionBegin(); partid < Env::GetPartitionEnd(); ++partid)
    {
        mWriterPtrs[partid % num_parts] = 
            new RecordWriter<IndexEntry>(Env::GetStep4InputName(partid));
    }
}

NewRefAccumulator::~NewRefAccumulator()
{
    int num_parts = Env::GetNumPartitionsPerNode();
    for (int partid = Env::GetPartitionBegin(); partid < Env::GetPartitionEnd(); ++partid)
    {
        delete mWriterPtrs[partid % num_parts];
    }
    delete[] mWriterPtrs;
}

void NewRefAccumulator::ProcessBuffer()
{
    int num_parts = Env::GetNumPartitionsPerNode();
    int num_records = 0;
    while (GetRecord()) {
        IndexEntry* p_record = static_cast<IndexEntry*>(mRecord);
        int part_id = Env::GetPartitionId(p_record->mCksum);
        mWriterPtrs[part_id % num_parts]->Put(*p_record);
        num_records++;
    }
    LOG_DEBUG("processed " << num_records << " records");
    Reset();
}


DupRecordAccumulator::DupRecordAccumulator()
{
    mRecord = static_cast<DataRecord*>(new BlockMeta);
    mRecordSize = mRecord->GetSize();
    for (int i = 0; Env::GetVmId(i) >= 0; i++) {
        int vmid = Env::GetVmId(i);
        mWriters[vmid] = new RecordWriter<BlockMeta>(Env::GetStep3OutputName(vmid), true);
        //mWriters[vmid] = new RecordWriter<BlockMeta>(Env::GetStep4OutputName(vmid));
    }
}

DupRecordAccumulator::~DupRecordAccumulator()
{
    map<int, RecordWriter<BlockMeta>*>::iterator it;
    for (it = mWriters.begin(); it != mWriters.end(); it++) {
        delete it->second;
    }
}

void DupRecordAccumulator::ProcessBuffer()
{
    int num_records = 0;
    map<int, RecordWriter<BlockMeta>*>::iterator it;

    while (GetRecord()) {
        BlockMeta* p_record = static_cast<BlockMeta*>(mRecord);
        it = mWriters.find(p_record->mBlk.mFileID);
        if (it == mWriters.end()) {
            LOG_ERROR("this block does not belong to current node " << p_record->mBlk.mFileID);
        }
        else {
            it->second->Put(*p_record);
            num_records++;
        }
    }
    LOG_DEBUG("processed " << num_records << " records");
    Reset();
}












