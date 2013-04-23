#include "trace_types.h"
#include <string.h>
#include <stdio.h>
#include <sstream>

const uint32_t RECORD_SIZE = 36;			// size of each block in scan log
const uint32_t FIX_SEGMENT_SIZE = (2 * 1024 * 1024);	// size of a segment
const uint32_t AVG_BLOCK_SIZE = (4 * 1024);
const uint16_t SENDER_HAS_DATA = 0x1;
const uint16_t SENDER_HAS_NO_DATA = 0x2;

MsgHeader::MsgHeader()
	: mTotalSize(0),
      mNumRecords(0),
      mRecordSize(0),
      mFlags(SENDER_HAS_DATA)
{
}

int MsgHeader::GetSize()
{
    return sizeof(MsgHeader::mTotalSize) + sizeof(MsgHeader::mNumRecords)
        + sizeof(MsgHeader::mRecordSize) + sizeof(MsgHeader::mFlags);
}

void MsgHeader::ToBuffer(char* buf)
{
    memcpy(buf, (char*)&mTotalSize, sizeof(mTotalSize));
    buf += sizeof(mTotalSize);
    memcpy(buf, (char*)&mNumRecords, sizeof(mNumRecords));
    buf += sizeof(mNumRecords);
    memcpy(buf, (char*)&mRecordSize, sizeof(mRecordSize));
    buf += sizeof(mRecordSize);
    memcpy(buf, (char*)&mFlags, sizeof(mFlags));
}

void MsgHeader::FromBuffer(char* buf)
{
    memcpy((char*)&mTotalSize, buf, sizeof(mTotalSize));
    buf += sizeof(mTotalSize);
    memcpy((char*)&mNumRecords, buf, sizeof(mNumRecords));
    buf += sizeof(mNumRecords);
    memcpy((char*)&mRecordSize, buf, sizeof(mRecordSize));
    buf += sizeof(mRecordSize);
    memcpy((char*)&mFlags, buf, sizeof(mFlags));
}    

Checksum::Checksum()
{
    memset(mData, '\0', CKSUM_LEN);
}

Checksum& Checksum::operator=(const Checksum& other)
{
    if (this == &other)
        return *this;
    memcpy(mData, other.mData, CKSUM_LEN);
    return *this;
}

bool Checksum::operator==(const Checksum& other) const
{
    return memcmp(mData, other.mData, CKSUM_LEN) == 0;
}

bool Checksum::operator!=(Checksum const &other) const
{
    return memcmp(mData, other.mData, CKSUM_LEN) != 0;
}

bool Checksum::operator<(const Checksum& other) const
{
    return memcmp(mData, other.mData, CKSUM_LEN) < 0;
}

void Checksum::ToStream(ostream& os) const
{
    os.write((char*)mData, CKSUM_LEN);
}

bool Checksum::FromStream(istream& is)
{
    is.read((char*)mData, CKSUM_LEN);
    if (is.gcount() != CKSUM_LEN)
        return false;
    return true;
}

void Checksum::ToBuffer(char *buf)
{
    memcpy(buf, (char*)mData, CKSUM_LEN);
}

void Checksum::FromBuffer(char *buf)
{
    memcpy((char*)mData, buf, CKSUM_LEN);
}

string Checksum::ToString()
{
    string s(2 * CKSUM_LEN, '0');
    for (size_t i = 0; i < CKSUM_LEN; i++) {
		sprintf(&s[2 * i], "%02x", mData[i]);
	}
	return s;
}

uint32_t Checksum::First4Bytes() const
{
    uint32_t tmp;
    memcpy((char*)&tmp, mData, 4);
    return tmp;
}

uint32_t Checksum::Last4Bytes() const
{
    uint32_t tmp;
    memcpy((char*)&tmp, &mData[CKSUM_LEN - 4], 4);
    return tmp;
}

int Block::GetSize()
{
    return CKSUM_LEN + sizeof(Block::mFileID) + sizeof(Block::mFlags) + sizeof(Block::mSize), sizeof(Block::mOffset);
}

Block::Block(int size, const Checksum& ck) 
{
    mSize = size;
    mCksum = ck;
}

void Block::ToStream(ostream& os) const
{
    mCksum.ToStream(os);
    os.write((char *)&mFileID, sizeof(Block::mFileID));
    os.write((char *)&mFlags, sizeof(Block::mFlags));
    os.write((char *)&mSize, sizeof(Block::mSize));
    os.write((char *)&mOffset, sizeof(Block::mOffset));
}

bool Block::FromStream(istream& is) 
{
    if (!mCksum.FromStream(is))
        return false;

    int nbytes = RECORD_SIZE - CKSUM_LEN;
    char buf[RECORD_SIZE - CKSUM_LEN];
    is.read(buf, nbytes);
    if (is.gcount() != nbytes)
        return false;
    memcpy(&mFileID, buf, sizeof(Block::mFileID));
    memcpy(&mFlags, buf + sizeof(Block::mFileID), sizeof(Block::mFlags));
    memcpy(&mSize, buf + sizeof(Block::mFileID) + sizeof(Block::mFlags), sizeof(Block::mSize));
    memcpy(&mOffset, buf + sizeof(Block::mFileID) + sizeof(Block::mFlags) + sizeof(Block::mSize), sizeof(Block::mOffset));
    return true;
}

void Block::ToBuffer(char *buf)
{
    mCksum.ToBuffer(buf);
    buf += CKSUM_LEN;
    memcpy(buf, (char*)&mFileID, sizeof(Block::mFileID));
    buf += sizeof(Block::mFileID);
    memcpy(buf, (char*)&mFlags, sizeof(Block::mFlags));
    buf += sizeof(Block::mFlags);
    memcpy(buf, (char*)&mSize, sizeof(Block::mSize));
    buf += sizeof(Block::mSize);
    memcpy(buf, (char*)&mOffset, sizeof(Block::mOffset));
}

void Block::FromBuffer(char* buf)
{
    mCksum.FromBuffer(buf);
    buf += CKSUM_LEN;
    memcpy((char*)&mFileID, buf, sizeof(Block::mFileID));
    buf += sizeof(Block::mFileID);
    memcpy((char*)&mFlags, buf, sizeof(Block::mFlags));
    buf += sizeof(Block::mFlags);
    memcpy((char*)&mSize, buf, sizeof(Block::mSize));
    buf += sizeof(Block::mSize);
    memcpy((char*)&mOffset, buf, sizeof(Block::mOffset));
}    

string Block::ToString()
{
    stringstream ss;
    ss << "checksum: " << mCksum.ToString();    
    ss << " offset: " << mOffset;
    ss << " size: " << mSize;
    ss << " file_id: " << mFileID;
    ss << " flags: " << mFlags;
    return ss.str();
}

bool Block::operator==(const Block& other) const 
{
    return mCksum == other.mCksum;
}

bool Block::operator!=(const Block& other) const 
{
    return mCksum != other.mCksum;
}

bool Block::operator<(const Block& other) const 
{
    return mCksum < other.mCksum;
}

Segment::Segment() 
{
    mMinIndex = 0;
    mSize = 0;
}

void Segment::Init() 
{
    mMinIndex = 0;
    mSize = 0;
    mCtx = new SHA_CTX;
    SHA1_Init(mCtx);
    mBlocklist.clear();
    mBlocklist.reserve(256);
}

void Segment::AddBlock(const Block& blk) 
{
    mBlocklist.push_back(blk);
    mSize += blk.mSize;
    SHA1_Update(mCtx, blk.mCksum.mData, CKSUM_LEN);
    if (blk.mCksum < mBlocklist[mMinIndex].mCksum)
        mMinIndex = mBlocklist.size() - 1;
}

void Segment::Final() 
{
    SHA1_Final((unsigned char*)mCksum.mData, mCtx);
    delete mCtx;
}

uint64_t Segment::GetOffset() 
{
    if (mBlocklist.size() == 0)
        return 0;
    return mBlocklist[0].mOffset;
}

// minhash value can be used this way
Checksum& Segment::GetMinHash() 
{
    return mBlocklist[mMinIndex].mCksum;
}

void Segment::ToStream(ostream &os) const
{
    uint32_t num_blocks = mBlocklist.size();
    os.write((char *)&num_blocks, sizeof(uint32_t));
    for (uint32_t i = 0; i < num_blocks; i ++)
        mBlocklist[i].ToStream(os);
}

bool Segment::FromStream(istream &is)
{
    Block blk;
    uint32_t num_blocks;
    Init();
        
    is.read((char *)&num_blocks, sizeof(uint32_t));
    if (is.gcount() != sizeof(uint32_t))
        return false;
    for (uint32_t i = 0; i < num_blocks; i ++) {
        if (blk.FromStream(is))
            AddBlock(blk);
        else
            return false;
    }

    Final();
    return true;
}

bool Segment::LoadFixSize(istream &is)
{
    Block blk;
    Init();
    while (blk.FromStream(is)) {
        if (blk.mSize == 0) {       // fix the zero-sized block bug in scanner
            continue;
        }
        AddBlock(blk);
        if (mSize >= FIX_SEGMENT_SIZE)
            break;
    }
    Final();
    if (mSize == 0)
        return false;
    else
        return true;
}

void Segment::SaveBlockList(ostream &os) const
{
    for (uint32_t i = 0; i < mBlocklist.size(); i ++)
        mBlocklist[i].ToStream(os);
}


bool Segment::operator==(const Segment& other) const 
{
    return mCksum == other.mCksum;
}
