#ifndef _DEDUP_BUFFER_H_
#define _DEDUP_BUFFER_H_

#include <set>
#include <string>
#include "snapshot_meta.h"

using namespace std;

class DedupBuffer
{
public:
    void ToStream(ostream& os);
    void FromStream(istream& is);

    void ToFile(const string& fname);
    void FromFile(const string& fname);

    void Add(const Block& blk);

    bool Find(const Block& blk);

private:
    set<Block> mBlockSet;
};

#endif










