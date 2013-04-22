#include "dedup_buffer.h"
#include "disk_io.h"
#include <fstream>

void DedupBuffer::ToStream(ostream &os)
{
    set<Block>::iterator it;
    for (it = mBlockSet.begin(); it != mBlockSet.end(); it++) {
        it->ToStream(os);
    }
}

void DedupBuffer::FromStream(istream &is)
{
    Block blk;
    while(blk.FromStream(is)) {
        mBlockSet.insert(blk);
    }
}

void DedupBuffer::ToFile(string const &fname)
{
    RecordWriter<Block> writer(fname);
    set<Block>::iterator it;
    for (it = mBlockSet.begin(); it != mBlockSet.end(); it++) {
        writer.Put(*it);
    }
}
    
void DedupBuffer::FromFile(string const &fname)
{
    Block blk;
    RecordReader<Block> reader(fname);
    while(reader.Get(blk)) {
        mBlockSet.insert(blk);
    }
}

void DedupBuffer::Add(Block const &blk)
{
    mBlockSet.insert(blk);
}

bool DedupBuffer::Find(Block const &blk)
{
    set<Block>::iterator it = mBlockSet.find(blk);
    return (it != mBlockSet.end());
}
       
    




















