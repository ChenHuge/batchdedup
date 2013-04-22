#include <fstream>
#include "snapshot_mixer.h"
#include <string.h>

SnapshotMixer::SnapshotMixer(int vmid, int ssid, const string& input, const string& output)
    : mVmId(vmid),
      mSsId(ssid),
      mInputFile(input),
      mOutputFile(output)
{
}

bool SnapshotMixer::Generate()
{
    ifstream is(mInputFile.c_str(), ios::in | ios::binary);
    ofstream os(mOutputFile.c_str(), ios::out | ios::binary | ios::trunc);
    if (!is.is_open() || !os.is_open()) {
        return false;
    }
    
    int    seed          = mVmId * MAX_NUM_SNAPSHOTS;
    double rd            = 0.0;
    double seg_threshold = 0.0;
    double blk_threshold = 0.0;

    srand(seed);
    Segment seg;

    while (seg.LoadFixSize(is)) {
        for (int i = 0; i <= mSsId; i++) {
            if (i == 0) {
                seg_threshold = VM_SEG_CHANGE_RATE;
                blk_threshold = VM_BLOCK_CHANGE_RATE;
            }
            else {
                seg_threshold = SS_SEG_CHANGE_RATE;
                blk_threshold = SS_BLOCK_CHANGE_RATE;
            }

            rd = (double)rand() / RAND_MAX;
            if (rd > seg_threshold) { // keep this segment
                for (size_t j = 0; j < seg.mBlocklist.size(); j++) {
                    if (i == 0) {
                        seg.mBlocklist[j].mFlags = BLOCK_DIRTY_FLAG;	// first snapshot is always dirty
                    }
                    else {
                        seg.mBlocklist[j].mFlags = BLOCK_CLEAN_FLAG;
                    }
                }
                continue;
            }
            for (size_t j = 0; j < seg.mBlocklist.size(); j++) {
                rd = (double)rand() / RAND_MAX;
                if (rd > blk_threshold) { // keep this block
                    continue;
                }
                // fake a checksum
                rd = (double)rand() / RAND_MAX;
                memcpy(&seg.mBlocklist[j].mCksum.mData[4], &rd, sizeof(rd));	// mix random data into checksum
                seg.mBlocklist[j].mFlags = BLOCK_DIRTY_FLAG;	// all blocks in this segment are dirty
            }
        }

        // update the VM ID
        for (size_t j = 0; j < seg.mBlocklist.size(); j++) {
            seg.mBlocklist[j].mFileID = mVmId;
        }

        seg.SaveBlockList(os);
    }

    is.close();
    os.close();
    return true;
}



















