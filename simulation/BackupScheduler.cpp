#include "BackupScheduler.h"


void BackupScheduler::setMachineList(vector<vector<double> > machine_loads) {
    machines=machine_loads;
}


bool NullScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.size() < 1) {
        return false;
    }
    bool schedule = false;
    for(vector<vector<double> >::const_iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        if ((*machine).size() > 0) {
            schedule = true;
            break;
        }
    }
    if (!schedule) {
        return false;
    }
    round_schedule.insert(round_schedule.end(),machines.begin(), machines.end());
    machines.clear();
    return true;
}

const char * NullScheduler::getName() {
    return "Null Scheduler";
}

bool OneEachScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.size() < 1) {
        return false;
    }
    bool schedule = false;
    for(vector<vector<double> >::const_iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        if ((*machine).size() > 0) {
            schedule = true;
            break;
        }
    }
    if (!schedule) {
        return false;
    }

    for(vector<vector<double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        vector<double> machine_schedule;
        if ((*machine).size() > 0) {
            machine_schedule.push_back((*machine).back());
            (*machine).pop_back();
        }
        round_schedule.push_back(machine_schedule);
    }
    return true;
}

const char * OneEachScheduler::getName() {
    return "One Each Scheduler";
}

bool OneScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.size() < 1) {
        return false;
    }
    bool schedule = false;
    for(vector<vector<double> >::const_iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        if ((*machine).size() > 0) {
            schedule = true;
            break;
        }
    }
    if (!schedule) {
        return false;
    }

    bool scheduled = false;
    for(vector<vector<double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        vector<double> machine_schedule;
        if (!scheduled && (*machine).size() > 0) {
            scheduled = true;
            machine_schedule.push_back((*machine).back());
            (*machine).pop_back();
        }
        round_schedule.push_back(machine_schedule);
    }
    return true;
}

const char * OneScheduler::getName() {
    return "One Scheduler";
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
