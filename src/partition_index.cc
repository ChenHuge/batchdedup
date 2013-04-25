#include "partition_index.h"
#include "batch_dedup_config.h"

#include <fstream>
#include <algorithm>

int IndexEntry::GetSize()
{
    return CKSUM_LEN + mRef.GetSize();
}

void IndexEntry::FromBuffer(char* buf)
{
    mCksum.FromBuffer(buf);
    buf += CKSUM_LEN;
    mRef.FromBuffer(buf);
}

void IndexEntry::ToBuffer(char *buf)
{
    mCksum.ToBuffer(buf);
    buf += CKSUM_LEN;
    mRef.ToBuffer(buf);
}

bool IndexEntry::FromStream(istream &is)
{
    return mCksum.FromStream(is) && mRef.FromStream(is);
}

void IndexEntry::ToStream(ostream& os) const
{
    mCksum.ToStream(os);
    mRef.ToStream(os);
}

bool IndexEntry::operator<(const IndexEntry& other) const
{
    return mCksum < other.mCksum;
}

bool IndexEntry::operator==(const IndexEntry& other) const
{
    return mCksum == other.mCksum;
}

void PartitionIndex::FromStream(istream &is)
{
    mIndex.clear();
    IndexEntry ie;
    while (ie.FromStream(is)) {
        mIndex.push_back(ie);
    }
    sort(mIndex.begin(), mIndex.end());
}

void PartitionIndex::ToStream(ostream &os)
{
    for (size_t i = 0; i < mIndex.size(); i++) {
        mIndex[i].ToStream(os);
    }
}

void PartitionIndex::FromFile(const string& fname)
{
    ifstream is(fname.c_str(), ios::in | ios::binary);
    if (!is.is_open()) {
        LOG_DEBUG("can not open " << fname << ", partition index is set to empty");
        return;
    }
    is.seekg(0, is.end);
    size_t len = is.tellg();
    is.seekg(0, is.beg);
    size_t sz = len / (sizeof(Checksum) + sizeof(DataRef));
    mIndex.reserve(sz);
    FromStream(is);
    is.close();
    LOG_DEBUG("loaded " << mIndex.size() << " items from " << fname);
}

void PartitionIndex::ToFile(const string& fname)
{
    ofstream os(fname.c_str(), ios::out | ios::binary | ios::app);	// append or overwrite?
    ToStream(os);
    os.close();
}

bool PartitionIndex::Find(Checksum const &cksum)
{
    IndexEntry ie;
    ie.mCksum = cksum;
    return binary_search(mIndex.begin(), mIndex.end(), ie);
}
















