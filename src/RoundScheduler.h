#include <vector>

using namespace std;

/*extern double total_size(const vector<vector<double> > &machine_loads);
extern double model_time(const vector<vector<double> > &machine_loads, bool verbose);
extern double model_time2(const vector<vector<double> > &machine_loads, int mid, int vmsize, bool verbose);
extern double model_vm_time(double vm_load, bool verbose);
extern double model_cow(double size, double block_dirty_ratio, double backup_time);
extern double model_unneccessary_cow(double size, double block_dirty_ratio, double backup_time);
extern double model_round_cow(const vector<vector<double> > &machine_loads);
extern double measure_load(const vector<vector<double> > &machine_loads, double &max_size, int &max_mid);
extern double model_time(double max_size, double total_size, int p, bool verbose);*/

class BackupScheduler {
    public:
        void setMachineList(std::vector<std::vector<double> > machine_loads);
        void setTimeLimit(double seconds);
        virtual bool schedule_round(std::vector<std::vector<int> > &round_schedule) = 0;
        virtual const char * getName() = 0;
    protected:
        std::vector<std::vector<double> > machines;
        double time_limit;
};

class NullScheduler : public BackupScheduler{
    public:
        bool schedule_round(std::vector<std::vector<double> > &round_schedule);
        const char * getName();
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

class DBPScheduler2 : public BackupScheduler{
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
