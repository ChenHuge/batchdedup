#include "snapshot_meta.h"
#include "string.h"

const DataRef INVALID_REF(0xFFFFFFFFFFFFFFFF);
const DataRef EMPTY_REF(0xAAAAAAAAAAAAAAAA);
const DataRef VALID_REF(0xBBBBBBBBBBBBBBBB);
const DataRef FOUND_REF(0xCCCCCCCCCCCCCCCC);

DataRef::DataRef() 
{
}

DataRef::DataRef(uint64_t data) 
{ 
    mData = data; 
}

int DataRef::GetSize()
{
    return sizeof(DataRef::mData);
}

void DataRef::ToStream(ostream& os) const
{
    os.write((char*)&mData, sizeof(mData));
}

bool DataRef::FromStream(istream& is)
{
    uint64_t tmp;
    is.read((char*)&tmp, sizeof(tmp));
    if (is.gcount() != sizeof(tmp)) {
        return false;
    }
    mData = tmp;
    return true;
}

void DataRef::ToBuffer(char *buf)
{
    memcpy(buf, (char*)&mData, sizeof(mData));
}

void DataRef::FromBuffer(char* buf)
{
    memcpy((char*)&mData, buf, sizeof(mData));
}

int BlockMeta::GetSize()
{
    return mBlk.GetSize() + mRef.GetSize();
}

void BlockMeta::ToStream(ostream& os)
{
    mBlk.ToStream(os);
    mRef.ToStream(os);
}

bool BlockMeta::FromStream(istream& is)
{
    return mBlk.FromStream(is) && mRef.FromStream(is);
}

void BlockMeta::ToBuffer(char *buf)
{
    mBlk.ToBuffer(buf);
    mRef.ToBuffer(buf + RECORD_SIZE);
}

void BlockMeta::FromBuffer(char* buf)
{
    mBlk.FromBuffer(buf);
    mRef.FromBuffer(buf + RECORD_SIZE);
}




















