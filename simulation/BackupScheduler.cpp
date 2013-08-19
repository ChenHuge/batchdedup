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

vector<double>::iterator find_max(vector<vector<double> > &machines, int &mid) {
    //cout << "finding vm with max ucow..." << endl;
    mid = 0;
    double schedule_time = model_time(machines, false);
    //cout << "  initializing max vm..." << endl;
    vector<double>::iterator max_vm = machines[0].end();
    for(int i = 0; i < machines.size(); i++) {
        if (!machines[i].empty()) {
            max_vm = machines[i].begin();
            mid = i;
            break;
        }
    }
    //cout << "  searching for max ucow..." << endl;
    
    double max_cow = model_unneccessary_cow(*max_vm, 0.2, schedule_time);
    for(int i = 0; i < machines.size(); i++) {
        for(vector<double>::iterator vm = machines[i].begin(); vm != machines[i].end(); ++vm) {
            double cow = model_unneccessary_cow(*vm, 0.2, schedule_time);
            if (cow > max_cow) {
                mid = i;
                max_vm = vm;
                max_cow = cow;
            }
        }
    }
    //cout << "returning vm with max ucow" << endl;
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
    for(vector<vector<double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        vector<double> vmschedule;
        machine_schedule.push_back(vmschedule);
    }
    for(int i = 1; i < rounds; i++) {
        round_schedules.push_back(machine_schedule);
    }

    //cout << "scheduling to one round..." << endl;

    int total_vms = 0;
    for(vector<vector<double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine)
    {
        total_vms += (*machine).size();
    }
    int to_move = total_vms - (total_vms / rounds); //how many vms to push off to a later round

    round_schedule.insert(round_schedule.end(), machines.begin(), machines.end());
    machines.clear();


    for(int i = 0; i < to_move; i++) {
        //cout << "scheduling vm..." << endl;
        int mid;
        vector<double>::iterator max_vm = find_max(round_schedule, mid);
        vector<vector<vector<double> > >::iterator schedule = round_schedules.begin();
        vector<vector<vector<double> > >::iterator min_schedule = round_schedules.begin();
        //cout << "finding round with smallest ucow..." << endl;
        //cout << "  initializing min ucow..." << endl;
        double min_cow = model_unneccessary_cow(*max_vm, 0.2, model_time(*min_schedule, false));
        while (schedule != round_schedules.end()) {
            //cout << "  modeling time..." << endl;
            double schedule_time = model_time(*schedule, false);
            //cout << "  modeling ucow..." << endl;
            double schedule_cow = model_unneccessary_cow(*max_vm, 0.2, schedule_time);
            if (schedule_cow < min_cow) {
                min_cow = schedule_cow;
                min_schedule = schedule;
            }
            ++schedule;
        }
        //cout << "scheduling vm to round..." << endl;
        (*min_schedule)[mid].push_back(*max_vm);
        //cout << "clearing vm from round 0..." << endl;
        //print_loads(round_schedules[0]);
        round_schedule[mid].erase(max_vm);
        //cout << "  done..." << endl;
    }
    cout << "loading round schedule..." << endl;

    cout << "returning round schedule..." << endl;

    return true;
}

const char * CowScheduler::getName() {
    return "CoW Scheduler";
}
