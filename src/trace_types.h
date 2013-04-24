#ifndef _TRACE_TYPES_H_
#define _TRACE_TYPES_H_

#include <cstdlib>
#include <stdint.h>	// for int types
#include <vector> 
#include <iostream>
#include <openssl/sha.h>

using namespace std;

#define CKSUM_LEN 20			// length of sha-1 checksum

extern const uint32_t RECORD_SIZE;			// size of each block in scan log
extern const uint32_t FIX_SEGMENT_SIZE;	// size of a segment
extern const uint32_t AVG_BLOCK_SIZE;
extern const uint16_t SENDER_HAS_DATA;
extern const uint16_t SENDER_HAS_NO_DATA;

class DataRecord
{
public:
    virtual ~DataRecord();

    virtual int GetSize() = 0;

    virtual void ToBuffer(char* buf) = 0;

    virtual void FromBuffer(char* buf) = 0;

    virtual void ToStream(ostream& os) const = 0;

    virtual bool FromStream(istream& is) = 0;
};

class MsgHeader
{
public:
    uint32_t mTotalSize;
    uint32_t mNumRecords;
    uint16_t mRecordSize;
    uint16_t mFlags;

public:
    MsgHeader();

    int GetSize();

    void ToBuffer(char* buf);
    void FromBuffer(char* buf);

    string ToString();
};

struct Checksum
{
    char mData[CKSUM_LEN];

    Checksum();

    Checksum& operator=(const Checksum& other);

    bool operator==(const Checksum& other) const;

    bool operator!=(const Checksum& other) const;

    bool operator<(const Checksum& other) const;

    void ToStream(ostream& os) const;

    bool FromStream(istream& is);

    void ToBuffer(char* buf);

    void FromBuffer(char* buf);

    string ToString();

    uint32_t First4Bytes() const;

    uint32_t Last4Bytes() const;
};

/*
 * store the information of a variable size block data
 */
class Block : public DataRecord
{
public:
	uint32_t mSize;
    uint16_t mFlags;
	uint16_t mFileID;
	uint64_t mOffset;
	Checksum mCksum;

public:
	Block() {};

	Block(int size, const Checksum& ck);

	~Block() {};

    int GetSize();

	void ToStream(ostream& os) const;

	bool FromStream(istream& is);

    void ToBuffer(char* buf);

    void FromBuffer(char* buf);

    string ToString();

	bool operator==(const Block& other) const;

	bool operator!=(const Block& other) const;

	bool operator<(const Block& other) const;
};

class Segment {
public:
	std::vector<Block> mBlocklist;
	uint32_t mMinIndex;	// location of min-hash block in mBlocklist
	uint32_t mSize;		// overall number of bytes
	Checksum mCksum;	// segment checksum is the SHA-1 of all block hash values

public:
	Segment();

	void Init();

	void AddBlock(const Block& blk);

	void Final();

    /*
     * return the offset of the first block
     */
	uint64_t GetOffset();

	/*
     * minhash value can be used this way
     */
	Checksum& GetMinHash();

    /*
     * Serialization of segment data
     * this is different from a scan trace
     */
    void ToStream(ostream& os) const;
	bool FromStream(istream& is);

    /*
     * Load a fixed segment from scan trace
     */
    bool LoadFixSize(istream& is);
    void SaveBlockList(ostream& os) const;

	bool operator==(const Segment& other) const;

private:
	SHA_CTX *mCtx;
};

#endif /* _TRACE_TYPES_H_ */
