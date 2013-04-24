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
    return Env::GetNodeId(pblk->mCksum);
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



















