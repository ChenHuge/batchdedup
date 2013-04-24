#ifndef _DISK_IO_H_
#define _DISK_IO_H_

#include "batch_dedup_config.h"
#include "trace_types.h"
#include <fstream>
#include <string>

using namespace std;

template<class T>
class RecordReader
{
public:
    RecordReader(const string& fname)
        : mBuffer(NULL),
          mFileName(fname)
    {
        mBuffer = new char[Env::GetReadBufSize()];
        mInput.rdbuf()->pubsetbuf(mBuffer, Env::GetReadBufSize());
        mInput.open(mFileName.c_str(), ios::in | ios::binary);
        if (mInput.is_open()) {
            LOG_DEBUG("open for read success" << fname);
        }
        else {
            LOG_DEBUG("open for read fail" << fname);
        }
    }

    ~RecordReader()
    {
        if (mInput.is_open()) {
            mInput.close();
        }
        if (mBuffer != NULL) {
            delete[] mBuffer;
        }
    }

    bool Get(T& record)
    {
        return record.FromStream(mInput);
    }

private:
    char* mBuffer;
    string mFileName;
    ifstream mInput;
};

template<class T>
class RecordWriter
{
public:
    RecordWriter(const string& fname, bool append = false)
        : mBuffer(NULL),
          mFileName(fname)
    {
        mBuffer = new char[Env::GetWriteBufSize()];
        mOutput.rdbuf()->pubsetbuf(mBuffer, Env::GetReadBufSize());
        if (append) {
            mOutput.open(mFileName.c_str(), ios::out | ios::binary | ios::app);
        }
        else {
            mOutput.open(mFileName.c_str(), ios::out | ios::binary | ios::trunc);
        }
        if (mOutput.is_open()) {
            LOG_DEBUG("open for write success" << fname);
        }
        else {
            LOG_DEBUG("open for write fail" << fname);
        }
    }

    ~RecordWriter()
    {
        if (mOutput.is_open()) {
            mOutput.close();
        }
        if (mBuffer != NULL) {
            delete[] mBuffer;
        }
    }

    void Put(const T& record)
    {
        record.ToStream(mOutput);
    }

private:
    char* mBuffer;
    string mFileName;
    ofstream mOutput;
};

#endif



