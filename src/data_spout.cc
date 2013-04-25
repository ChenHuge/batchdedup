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
}

TraceReader::~TraceReader()
{
    if (mInput.is_open())
        mInput.close();
    delete[] mReadBuf;
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
            LOG_INFO("now reading trace " << fname);
        }

        // find a dirty block
        while (mBlk.FromStream(mInput)) {
            if (mBlk.mFlags & BLOCK_DIRTY_FLAG) {
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
}

NewBlockReader::~NewBlockReader()
{
    if (mInputPtr != NULL) {
        delete mInputPtr;
    }
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
            mInputPtr = new RecordReader<Block>(Env::GetStep2Output3Name(mPartId));
            mPartId ++;
        }

        // read one block
        if (mInputPtr->Get(mBlk)) {
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
}

NewRefReader::~NewRefReader()
{
    if (mInputPtr != NULL) {
        delete mInputPtr;
    }
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
}

DupBlockReader::~DupBlockReader()
{
    if (mInputPtr != NULL) {
        delete mInputPtr;
    }
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
            mInputPtr = new RecordReader<BlockMeta>(Env::GetStep2Output1Name(mPartId));
            mPartId ++;
        }

        // read one block
        if (mInputPtr->Get(mRecord)) {
            pdata =  static_cast<DataRecord*>(&mRecord);
            return true;
        }

        // end of step2 output1 of current partition, close it
        delete mInputPtr;
        mInputPtr = NULL;
    }
}















