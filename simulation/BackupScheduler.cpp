#include <vector>
#include <iostream>
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
            //schedules vms in reverse order (because pop_back is cheaper than removing first element)
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

//picks the vm with the highest ucow
//for vms with equal ucow, picks the vm on the machine with the greatest load
vector<double>::iterator pick_vm(vector<vector<double> > &machines, int &mid) {
    mid = 0;
    double schedule_time = model_time(machines, false);
    vector<double>::iterator max_vm = machines[0].end();
    for(int i = 0; i < machines.size(); i++) {
        if (!machines[i].empty()) {
            max_vm = machines[i].begin();
            mid = i;
            break;
        }
    }
    
    double max_cow = model_unneccessary_cow(*max_vm, 0.2, schedule_time);
    double max_cow_load = 0;

    for(vector<double>::iterator vm = machines[mid].begin(); vm != machines[mid].end(); ++vm) {
        max_cow_load += *vm;
    }

    for(int i = 0; i < machines.size(); i++) {
        double machine_load = 0;
        for(vector<double>::iterator vm = machines[i].begin(); vm != machines[i].end(); ++vm) {
            machine_load += *vm;
        }
        for(vector<double>::iterator vm = machines[i].begin(); vm != machines[i].end(); ++vm) {
            double cow = model_unneccessary_cow(*vm, 0.2, schedule_time);
            if (cow > max_cow || (cow == max_cow && machine_load > max_cow_load)) {
                mid = i;
                max_vm = vm;
                max_cow = cow;
                max_cow_load = machine_load;
            }
        }
    }
    return max_vm;
}

void print_loads(const vector<vector<double> > &loads) {
    for(vector<vector<double> >::const_iterator machine = loads.begin(); machine != loads.end(); ++machine) {
        cout << "Machine: " << endl;
        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            cout << "  VM: " << *vm << endl;
        }
    }
    cout << "End Machines" << endl;
}

bool CowScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    
    if (machines.empty() && round_schedules.empty()) {
        return false;
    } else if (!round_schedules.empty()) {
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    }
    ////first we schedule everything
    //round_schedule.insert(round_schedule.end(),machines.begin(), machines.end());
    //machines.clear();

    //first we need to decide how many rounds can be used, then we actually break the work into rounds
    //when we break the work into rounds, we need to figure out how to split the rounds (an even split is probably poor)

    //cout << "creating empty schedules..." << endl;
    int rounds = 3; //arbitraily chosen, should be algorithmically chosen
    vector<vector<double> > machine_schedule;
    round_schedules.push_back(machine_schedule); //empty schedule which we will initially fill with everything
    for(vector<vector<double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        vector<double> vmschedule;
        machine_schedule.push_back(vmschedule);
    }
    int total_machines = machines.size();
    for(int i = 1; i < rounds; i++) {
        round_schedules.push_back(machine_schedule);
    }

    int total_vms = 0;
    for(vector<vector<double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine)
    {
        total_vms += (*machine).size();
    }

    round_schedules[0].insert(round_schedules[0].end(), machines.begin(), machines.end());
    machines.clear();


    vector<vector<vector<double> > >::iterator source_round = round_schedules.begin();
    vector<vector<vector<double> > >::iterator dest_round = round_schedules.begin();
    ++dest_round;

    for(int r = 1; r < rounds; r++) {
        int to_move = total_vms - r*(total_vms / rounds); //how many vms to push off to the next round
        for(int i = 0; i < to_move; i++) {
            int mid;
            vector<double>::iterator max_vm = pick_vm(*source_round, mid);
            (*dest_round)[mid].push_back(*max_vm);
            (*source_round)[mid].erase(max_vm);
        }
        ++source_round;
        ++dest_round;
    }


    round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
    round_schedules.pop_back();

    return true;
}

const char * CowScheduler::getName() {
    return "CoW Scheduler";
}
