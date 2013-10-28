#ifndef ROUNDSCHEDULER_H
#include <vector>
#include <map>

using namespace std;

//#define DEFAULT_TIME_LIMIT (60*60*3)
#define DEFAULT_TIME_LIMIT (60)
//#define DEFAULT_DIRTY_RATIO 0.10
#define DEFAULT_SEGMENT_DIRTY_RATIO 0.2
#define DEFAULT_FP_RATIO 0.5
#define DEFAULT_DUPNEW_RATIO 0.01
#define DEFAULT_NETWORK_LATENCY 0.0005
#define DEFAULT_READ_BANDWIDTH (50.0 * BANDWIDTH_UNIT)
#define DEFAULT_DISK_WRITE_BANDWIDTH (50.0 * BANDWIDTH_UNIT)
//#define DEFAULT_BACKEND_WRITE_BANDWIDTH 90000000.0
#define DETECTION_REQUEST_SIZE 40.0
#define DUPLICATE_SUMMARY_SIZE 48.0
//4KB chunks
#define CHUNK_SIZE 4096.0
//2MB segments
#define SEGMENT_SIZE (512.0*CHUNK_SIZE)
//we define sizes in GB
#define SIZE_UNIT (1<<30)
//memory usage defined in MB
#define MEMORY_UNIT (1<<20)
//bandwidth defined in MB/s
#define BANDWIDTH_UNIT (1<<20)
#define DEFAULT_LOOKUP_TIME 0.000001
//manually computed from x=10,fp=0.5,dirty=0.2,chunk=4096,s=40GB,v=25
#define DEFAULT_MACHINE_INDEX_SIZE (262144000)
//25MB of net memory leaves 10MB for index parititions, r/w buffers, etc
#define DEFAULT_NETWORK_MEMORY (25.0 * MEMORY_UNIT)
//default write rate defines how many segments are written per second during snapshotting
//20.5 is from 24hr_1GBWrt_all in the EMC paper (the avg writes/second)
//issues with 20.5:
//  probably some writes during a second fall in the same segment, so should be lower (but this is an upper bound)
//  is an average over 24.4 hours; during the backup period (late night) value should be lower
//  number is for general data drives, where we really want vm drives (may increase/decrease value)
//#define DEFAULT_WRITE_RATE 20.5
//I am going with 10 for now just because I think 20.5 is too high
#define DEFAULT_WRITE_RATE 10.0

//cow constant goes into the exponent of the cow calcluation
//this constant still needs to be determined, as it is important to CoW cost
#define COW_CONSTANT 0.5


//double total_size(const vector<vector<double> > &machine_loads);
double model_time(const vector<vector<double> > &machine_loads, bool verbose);
double model_time2(const vector<vector<double> > &machine_loads, int mid, int vmsize, bool verbose);
/*
double model_vm_time(double vm_load, bool verbose);
double model_cow(double size, double block_dirty_ratio, double backup_time);
double model_unneccessary_cow(double size, double block_dirty_ratio, double backup_time);
double model_round_cow(const vector<vector<double> > &machine_loads);
*/
double measure_load(const vector<vector<double> > &machine_loads, double &max_size, int &max_mid);

double model_time(double max_size, double total_size, int p, bool verbose);

class RoundScheduler {
    public:
        void setMachineList(std::vector<std::map<int, double> > machine_loads);
        void setTimeLimit(double seconds);
        virtual bool schedule_round(std::vector<std::vector<int> > &round_schedule) = 0;
        virtual const char * getName() = 0;
    protected:
        std::vector<std::map<int,double> > machines;
        double time_limit;
};

class NullScheduler : public RoundScheduler{
    public:
        bool schedule_round(std::vector<std::vector<int> > &round_schedule);
        const char * getName();
};

class DBPScheduler2 : public RoundScheduler{
    public:
        bool schedule_round(std::vector<std::vector<int> > &round_schedule);
        const char * getName();
    private:
        vector<vector<vector<int> > > round_schedules;
        double pack_vms(vector<map<int, double> > machines,int rounds);
        void schedule_vms(vector<map<int, double> > &machines);
};

/*class OneEachScheduler : public BackupScheduler{
    public:
        bool schedule_round(std::vector<std::vector<double> > &round_schedule);
        const char * getName();
};
        
class OneScheduler : public BackupScheduler{
    public:
        bool schedule_round(std::vector<std::vector<double> > &round_schedule);
        const char * getName();
};
        
class CowScheduler : public BackupScheduler{
    public:
        bool schedule_round(std::vector<std::vector<double> > &round_schedule);
        const char * getName();
    private:
        vector<vector<vector<double> > > round_schedules;
};

class DBPScheduler : public BackupScheduler{
    public:
        bool schedule_round(std::vector<std::vector<double> > &round_schedule);
        const char * getName();
    private:
        vector<vector<vector<double> > > round_schedules;
        double pack_vms(vector<vector<double> > machines,int rounds);
        void schedule_vms(vector<vector<double> > &machines);
};

class DBPN1Scheduler : public BackupScheduler{
    public:
        bool schedule_round(std::vector<std::vector<double> > &round_schedule);
        const char * getName();
    private:
        vector<vector<vector<double> > > round_schedules;
        double pack_vms(vector<vector<double> > machines,int rounds);
        void schedule_vms(vector<vector<double> > &machines);
};

class DBPN2Scheduler : public BackupScheduler{
    public:
        bool schedule_round(std::vector<std::vector<double> > &round_schedule);
        const char * getName();
    private:
        vector<vector<vector<double> > > round_schedules;
        double pack_vms(vector<vector<double> > machines,int rounds);
        void schedule_vms(vector<vector<double> > &machines);
};*/
#endif
