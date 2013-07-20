#ifndef _PARTITION_INDEX_H_
#define _PARTITION_INDEX_H_

#include "trace_types.h"
#include "snapshot_meta.h"

using namespace std;

class IndexEntry : public DataRecord
{
public:
    Checksum mCksum;
    DataRef mRef;

public:
    int GetSize();

    void FromBuffer(char* buf);
    void ToBuffer(char* buf);

    bool FromStream(istream& is);
    void ToStream(ostream& os) const;

    bool operator<(const IndexEntry& other) const;
    bool operator==(const IndexEntry& other) const;
};

class PartitionIndex
{
public:
    uint32_t mNodeID;
    uint32_t mPartID;
    vector<IndexEntry> mIndex;

public:
    size_t getNumEntries();
    void FromStream(istream& is);
    void ToStream(ostream& os);

    void FromFile(const string& fname);
    void ToFile(const string& fname);
    
    void AppendToFile(const string& fname);

    bool Find(const Checksum& cksum);
};

#endif

















