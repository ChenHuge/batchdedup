#include <vector>

using namespace std;

class BackupScheduler {
    public:
        void setMachineList(std::vector<std::vector<double> > machine_loads);
        virtual bool schedule_round(std::vector<std::vector<double> > &round_schedule) = 0;
        virtual const char * getName() = 0;
    protected:
        std::vector<std::vector<double> > machines;
};

class AllScheduler : public BackupScheduler{
    public:
        bool schedule_round(std::vector<std::vector<double> > &round_schedule);
        const char * getName();
};
        
