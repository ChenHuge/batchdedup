#include <string>
#include "data_spout.h"
#include "batch_dedup_config.h"

DataSpout::~DataSpout()
{
}

TraceReader::TraceReader()
{
    mVmIdx = 0;
    mNumTasks = Env::GetNumTasks();
    mReadBuf = new char[Env::GetReadBufSize()];
    mInput.rdbuf()->pubsetbuf(mReadBuf, Env::GetReadBufSize());
    mStatTotalSize = 0;
    mStatTotalCount = 0;
    mStatDirtySize = 0;
}

TraceReader::~TraceReader()
{
    if (mInput.is_open())
        mInput.close();
    delete[] mReadBuf;
}

void TraceReader::Stat()
{
    LOG_INFO("total VM size: " << mStatTotalSize << ", dirty segment size: " << mStatDirtySize);
    LOG_INFO("total Blocks Read: " << mStatTotalCount);
}

int TraceReader::GetRecordSize()
{
    return mBlk.GetSize();
}

int TraceReader::GetRecordDest(DataRecord* pdata)
{
    Block* pblk = static_cast<Block*>(pdata);
    return Env::GetDestNodeId(pblk->mCksum);
}

// TODO: change to RecordReader
bool TraceReader::GetRecord(DataRecord*& pdata)
{
    while (true) {
        // open a VM trace
        if (!mInput.is_open()) {
            int vmid = Env::GetVmId(mVmIdx);
            if (vmid < 0) {
                return false;
            }
            mVmIdx ++;
            string fname = Env::GetVmTrace(vmid);
            mInput.open(fname.c_str(), ios::in | ios::binary);
            if (!mInput.is_open()) {
                LOG_ERROR("can not open trace: " << fname);
                return false;
            }
            cout << "M" << Env::GetRank() << " reading vm " << vmid;
            LOG_DEBUG("now reading trace " << fname);
        }

        // find a dirty block
        while (mBlk.FromStream(mInput)) {
            mStatTotalSize += (uint64_t)mBlk.mSize;
            mStatTotalCount++;
            if (mBlk.mFlags & BLOCK_DIRTY_FLAG) {
                mStatDirtySize += (uint64_t)mBlk.mSize;
                pdata = static_cast<DataRecord*>(&mBlk);
                return true;
            }
        }
        // end of VM trace, close it
        mInput.close();
    }
}


NewBlockReader::NewBlockReader()
{
    mPartId = Env::GetPartitionBegin();
    mInputPtr = NULL;
    mStatNewCount = 0;
}

NewBlockReader::~NewBlockReader()
{
    if (mInputPtr != NULL) {
        delete mInputPtr;
    }
}


void NewBlockReader::Stat()
{
    LOG_INFO("new blocks read: " << mStatNewCount);
}

int NewBlockReader::GetRecordSize()
{
    return mBlk.GetSize();
}

int NewBlockReader::GetRecordDest(DataRecord* pdata)
{
    Block* pblk = static_cast<Block*>(pdata);
    return Env::GetSourceNodeId(pblk->mFileID);	// send back to source
}

bool NewBlockReader::GetRecord(DataRecord *&pdata)
{
    while (true) {
        // open step2 output3 of current partition
        if (mInputPtr == NULL) {
            if (mPartId >= Env::GetPartitionEnd()) {
                return false;
            }
            mInputPtr = new RecordReader<Block>(Env::GetStep2OutputNewBlocksName(mPartId));
            mPartId ++;
        }

        // read one block
        if (mInputPtr->Get(mBlk)) {
            mStatNewCount++;
            pdata =  static_cast<DataRecord*>(&mBlk);
            return true;
        }

        // end of step2 output3 of current partition, close it
        delete mInputPtr;
        mInputPtr = NULL;
    }
}


NewRefReader::NewRefReader()
{
    mVmIdx = 0;
    mInputPtr = NULL;
    mStatNewSize = 0;
    mStatNewCount = 0;
}

NewRefReader::~NewRefReader()
{
    if (mInputPtr != NULL) {
        delete mInputPtr;
    }
}


void NewRefReader::Stat()
{
    LOG_INFO("new block size: " << mStatNewSize);
    LOG_INFO("new block count: " << mStatNewCount);
}

int NewRefReader::GetRecordSize()
{
    return mRecord.GetSize();
}

int NewRefReader::GetRecordDest(DataRecord* pdata)
{
    IndexEntry* pRecord = static_cast<IndexEntry*>(pdata);
    return Env::GetDestNodeId(pRecord->mCksum);
}

bool NewRefReader::GetRecord(DataRecord*& pdata)
{
    while (true) {
        // open an input
        if (mInputPtr == NULL) {
            int vmid = Env::GetVmId(mVmIdx);
            if (vmid < 0) {
                return false;
            }
            mVmIdx ++;
            mInputPtr = new RecordReader<BlockMeta>(Env::GetStep3OutputName(vmid));
        }

        // read a block meta
        BlockMeta bm;
        while (mInputPtr->Get(bm)) {
            mStatNewSize += bm.mBlk.mSize;
            mStatNewCount++;
            mRecord.mCksum = bm.mBlk.mCksum;
            mRecord.mRef = bm.mRef;
            pdata = static_cast<DataRecord*>(&mRecord);
            return true;
        }

        // end of input, close it
        delete mInputPtr;
        mInputPtr = NULL;
    }
}


DupBlockReader::DupBlockReader()
{
    mPartId = Env::GetPartitionBegin();
    mInputPtr = NULL;
    mStatDupCount = 0;
}

DupBlockReader::~DupBlockReader()
{
    if (mInputPtr != NULL) {
        delete mInputPtr;
    }
}


void DupBlockReader::Stat()
{
    LOG_INFO("dup blocks: " << mStatDupCount);
}

int DupBlockReader::GetRecordSize()
{
    return mRecord.GetSize();
}

int DupBlockReader::GetRecordDest(DataRecord* pdata)
{
    BlockMeta* p_record = static_cast<BlockMeta*>(pdata);
    return Env::GetSourceNodeId(p_record->mBlk.mFileID);	// send back to source
}

bool DupBlockReader::GetRecord(DataRecord *&pdata)
{
    while (true) {
        // open step2 output3 of current partition
        if (mInputPtr == NULL) {
            if (mPartId >= Env::GetPartitionEnd()) {
                return false;
            }
            mInputPtr = new RecordReader<BlockMeta>(Env::GetStep2OutputDupBlocksName(mPartId));
            mPartId ++;
        }

        // read one block
        if (mInputPtr->Get(mRecord)) {
            mStatDupCount++;
            pdata =  static_cast<DataRecord*>(&mRecord);
            return true;
        }

        // end of step2 output1 of current partition, close it
        delete mInputPtr;
        mInputPtr = NULL;
    }
}















