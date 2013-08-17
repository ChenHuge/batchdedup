#include <vector>

using namespace std;

extern double total_size(const vector<vector<double> > &machine_loads);
extern double model_time(const vector<vector<double> > &machine_loads, bool verbose);
extern double model_vm_time(double vm_load, bool verbose);
extern double model_cow(double size, double block_dirty_ratio, double backup_time);
extern double model_unneccessary_cow(double size, double block_dirty_ratio, double backup_time);
extern double model_round_cow(const vector<vector<double> > &machine_loads);

class BackupScheduler {
    public:
        void setMachineList(std::vector<std::vector<double> > machine_loads);
        virtual bool schedule_round(std::vector<std::vector<double> > &round_schedule) = 0;
        virtual const char * getName() = 0;
    protected:
        std::vector<std::vector<double> > machines;
};

class NullScheduler : public BackupScheduler{
    public:
        bool schedule_round(std::vector<std::vector<double> > &round_schedule);
        const char * getName();
};

class OneEachScheduler : public BackupScheduler{
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
};