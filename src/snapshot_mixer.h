#ifndef _SNAPSHOT_MIXER_H_
#define _SNAPSHOT_MIXER_H_

#include "stdlib.h"
#include "trace_types.h"
#include "batch_dedup_config.h"

using namespace std;

class SnapshotMixer
{
public:    
    SnapshotMixer(int vmid, int ssid, const string& input, const string& output);

    bool Generate();

private: 
    int    mVmId;
    int    mSsId;
    string mInputFile;
    string mOutputFile;
};

#endif
















