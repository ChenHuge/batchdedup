#ifndef _SNAPSHOT_META_H_
#define _SNAPSHOT_META_H_

#include <iostream>
#include "trace_types.h"

using namespace std;

struct DataRef;

extern const DataRef  INVALID_REF;
extern const DataRef  EMPTY_REF;
extern const DataRef  VALID_REF;
extern const DataRef  FOUND_REF;

struct DataRef
{
    DataRef();
    DataRef(uint64_t data);
    uint64_t mData;

    int GetSize();

    void ToStream(ostream& os) const;
    bool FromStream(istream& is);

    void ToBuffer(char* buf);
    void FromBuffer(char* buf);
};

class BlockMeta : public DataRecord
{
public:
    Block mBlk;
    DataRef mRef;

public:
    int GetSize();
    
    void ToStream(ostream& os) const;
    bool FromStream(istream& is);

    void ToBuffer(char* buf);
    void FromBuffer(char* buf);
};

#endif







