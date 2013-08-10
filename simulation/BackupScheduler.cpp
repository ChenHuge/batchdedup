#include "BackupScheduler.h"


void BackupScheduler::setMachineList(vector<vector<double> > machine_loads) {
    machines=machine_loads;
}


bool AllScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.size() > 0) {
        round_schedule.insert(round_schedule.end(),machines.begin(), machines.end());
        machines.clear();
        return true;
    } else {
        return false;
    }
}
const char * AllScheduler::getName() {
    return "All Scheduler";
}
