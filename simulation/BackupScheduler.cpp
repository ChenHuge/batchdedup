#include "BackupScheduler.h"


void BackupScheduler::setMachineList(vector<vector<double> > machine_loads) {
    machines=machine_loads;
}


bool NullScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.size() > 0) {
        round_schedule.insert(round_schedule.end(),machines.begin(), machines.end());
        machines.clear();
        return true;
    } else {
        return false;
    }
}

const char * NullScheduler::getName() {
    return "Null Scheduler";
}

bool CowScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.size() < 1)
        return false;
    //first we schedule everything
    round_schedule.insert(round_schedule.end(),machines.begin(), machines.end());
    machines.clear();
    //now we put off those vms with the worst uneccessary cow

    return true;
}

const char * CowScheduler::getName() {
    return "CoW Scheduler";
}
