#include <vector>
#include <iostream>
#include "BackupScheduler.h"


void BackupScheduler::setMachineList(vector<vector<double> > machine_loads) {
    machines=machine_loads;
}

void BackupScheduler::setTimeLimit(double seconds) {
    time_limit = seconds;
}


bool NullScheduler::schedule_round(std::vector<std::vector<int> > &round_schedule) {
    round_schedule.clear();
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
    for(int i = 0; i < machines.size(); i++) {
        vector<int> machine_schedule;
        for(int j = 0; j < machines[i].size; j++) {
            machine_schedule.insert(j);
        }
        round_schedule.insert(machine_schedule);
    }
    machines.clear();
    return true;
}

const char * NullScheduler::getName() {
    return "Null Scheduler";
}

/*bool OneEachScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
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

//picks the biggest vm (tiebreaker: left-most)
vector<double>::iterator pick_biggest_vm(vector<vector<double> > &machines, int &mid) {
    mid = 0;
    vector<double>::iterator max_vm = machines[0].end();
    for(int i = 0; i < machines.size(); i++) {
        if (!machines[i].empty()) {
            max_vm = machines[i].begin();
            mid = i;
            break;
        }
    }
    if (max_vm == machines[0].end()) {
        mid = -1;
        return max_vm;
    }
    
    for(int i = 0; i < machines.size(); i++) {
        for(vector<double>::iterator vm = machines[i].begin(); vm != machines[i].end(); ++vm) {
            if (*vm > *max_vm) {
                mid = i;
                max_vm = vm;
            }
        }
    }
    return max_vm;
}

//picks the vm which will decrease single-round time the most
//tie-breaker:picks the vm on the machine with the greatest load
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
    if (max_vm == machines[0].end()) {
        mid = -1;
        return max_vm;
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

//measures round duration after removing the specified vm from the specified machine
double model_rtime(const vector<vector<double> > &machines, int mid, int vm) {
    return model_time2(machines,mid,-vm,false);
}

//measures increase in round duration after adding the specified vm to the specified machine
double model_time_delta(const vector<vector<double> > &machines, int mid, int vm) {
    return model_time2(machines,mid,vm,false) - model_time(machines,false);
}

//measures the time of the round after adding the specified vm to the specified machine
double model_new_time(const vector<vector<double> > &machines, int mid, int vm) {
    return model_time2(machines,mid,vm,false);
}


//picks the vm whose removal will most decrease round time
//tie-breaker:machine with greatest load
vector<double>::iterator pick_max_tdelta_vm(vector<vector<double> > &machines, int &mid) {
    double max_size;
    int max_mid;
    double total_size = measure_load(machines, max_size,max_mid);
    double schedule_time = model_time(total_size, max_size, machines.size(), false);
    vector<double>::iterator best_vm = machines[0].end();
    mid = -1;
    
    double best_time;
    double best_time_load;

    for(int i = 0; i < machines.size(); i++) {
        double machine_load = 0;
        for(vector<double>::iterator vm = machines[i].begin(); vm != machines[i].end(); ++vm) {
            machine_load += *vm;
        }
        for(vector<double>::iterator vm = machines[i].begin(); vm != machines[i].end(); ++vm) {
            double rtime;
            //if we are removing from the max machine, we must recompute max_size
            if (max_mid == i) {
                rtime = model_rtime(machines, i, *vm);
            } else {
                rtime = model_time(total_size - *vm, max_size, machines.size() - 1, false);
            }
            //if we haven't picked yet, or if the single round time is lower by picking this VM, or if machine round is greater and it's a tie
            if (best_vm == machines[0].end() || rtime < best_time || (rtime == best_time && machine_load > best_time_load)) {
                mid = i;
                best_vm = vm;
                best_time = rtime;
                best_time_load = machine_load;
            }
        }
    }
    return best_vm;
}

//pick round for which this vm has the smallest ucow
vector<vector<vector<double> > >::iterator pick_min_ucow_round(vector<vector<vector<double> > > &round_schedules, const vector<vector<double> > &machines, int mid, vector<double>::const_iterator vm) {
    vector<vector<vector<double> > >::iterator best_round = round_schedules.begin();
    double best_ucow = model_unneccessary_cow(*vm, 0.2, model_time(*best_round,false));
    for(vector<vector<vector<double> > >::iterator round = round_schedules.begin()+1;
            round != round_schedules.end(); ++round) {
        double schedule_time = model_time(*round, false);
        double ucow = model_unneccessary_cow(*vm, 0.2, schedule_time);
        if (ucow < best_ucow) {
            best_round = round;
            best_ucow = ucow;
        }
    }
    return best_round;
}

//finds the round which will be the shortest in duration after adding the vm
vector<vector<vector<double> > >::iterator pick_min_newtime_round(vector<vector<vector<double> > > &round_schedules, const vector<vector<double> > &machines, int mid, vector<double>::const_iterator vm) {
    //double max_size;
    //int max_mid;
    //double total_size = measure_load(machines, max_size,max_mid);
    //double schedule_time = model_time(total_size, max_size, machines.size(), false);

    vector<vector<vector<double> > >::iterator best_round = round_schedules.end();
    double best_time;
    for(vector<vector<vector<double> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        double time = model_new_time(*round, mid, *vm);
        if (best_round == round_schedules.end() || time < best_time) {
            best_round = round;
            best_time = time;
        }
    }
    return best_round;
}

//finds the round which is currently shortest in duration
vector<vector<vector<double> > >::iterator pick_min_time_round(vector<vector<vector<double> > > &round_schedules, const vector<vector<double> > &machines, int mid, vector<double>::const_iterator vm) {
    vector<vector<vector<double> > >::iterator best_round = round_schedules.end();
    double best_time;
    for(vector<vector<vector<double> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        //double time = model_time_delta(*round, mid, *vm);
        double time = model_time(*round,false);
        if (best_round == round_schedules.end() || time < best_time) {
            best_round = round;
            best_time = time;
        }
    }
    return best_round;
}

double DBPScheduler::pack_vms(vector<vector<double> > machines,int rounds) {
    //basic LOWEST FIT DECREASING algorithm:
    //1. sort items in size-decreasing order
    //2. while there is an unpacked item
    //  a. pick the first unpacked item 'a'
    //  b. pick the bin with the lowest level 'b'
    //    i. in case of ties use left-most bin
    //  c. put item a into bin b
    //3. Halt

    //my packing algorithm (based on LFD)
    //1. while there is an unpacked item
    //  a. pick the vm with the highest ucow 'a'
    //    i. in case of ties take the vm from the machine with the highest load
    //  b. pick the round whose time will be lowest after adding a 'b'
    //    i. in case of ties pick the left-most round
    //  c. put item a into round b
    //2. Halt

    round_schedules.clear();
    vector<vector<double> > machine_schedule; //empty machine schedule
    vector<double> vmschedule; //empty vm schedule
    //fill machine_schedule with p empty vectors (one for each machine)
    for(int i = 0; i < machines.size(); i++) {
        machine_schedule.push_back(vmschedule);
    }
    //add r empty schedules
    for(int i = 0; i < rounds; i++) {
        round_schedules.push_back(machine_schedule);
    }



    vector<double>::iterator vm;
    vector<vector<vector<double> > >::iterator round;
    int mid;
    do {
        vm = pick_vm(machines,mid);
        if (vm == machines[0].end()) {
            break;
        }
        round = pick_min_newtime_round(round_schedules, machines, mid, vm);
        (*round)[mid].push_back(*vm);
        machines[mid].erase(vm);
    } while(true);

    double schedule_time = 0;
    for(vector<vector<vector<double> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        schedule_time += model_time(*round,false);
    }
    return schedule_time;
}

void DBPScheduler::schedule_vms(vector<vector<double> > &machines) {
    //basic ITERATED "A" algorithm:
    //1. Set UB=min(n,total_size/T)
    //2. Set LB=1
    //3. while UB-LB > 1
    //  a. Set N=(LB+UB)/2
    //  b. if A(machines,N) >= T, set LB=N
    //  c. else set UB=N
    //3. Return packing generated by A(machines,LB)
    //Note: A(I,N) returns the minimum bin level for packing set I into N bins

    //My scheduler (based on iterated "A"):
    //1. Set UB=min(n,2*total_vms/machine_count)
    //2. Set LB=1
    //3. while UB>LB
    //  a. set N = (UB+LB+1)/2
    //  b. if A(machines,N) > time_limit, set UB=N-1
    //  c. else set LB=N
    //4. Return packing generated by A(machines,UB)
    //Note: A(I,N) returns schedule duration for packing set I into N bins

    int total_count = 0;
    int total_size = 0;
    double schedule_time;
    for(vector<vector<double> >::iterator machine = machines.begin();
            machine != machines.end(); ++machine) {
        for(vector<double>::iterator vm = (*machine).begin();
                vm != (*machine).end(); ++vm) {
            total_count++;
            total_size += *vm;
        }
    }
    //int T = total_size/7; //in the original alg T was the goal
    //int UB = total_size/T;
    //if (UB > total_count) {
    //    UB = total_count;
    //}
    cout << "total count: " << total_count << "; total machines: " << machines.size() << endl;
    int UB = 2*total_count/machines.size(); //one vm per machine per round
    if (UB > total_count) {
        UB = total_count;
    }
    int LB = 1;
    while (UB > LB) {
        int N=(UB+LB+1)/2;
        schedule_time = pack_vms(machines,N);
        cout << "rounds: " << N << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
        if (schedule_time > time_limit) {
            UB=N-1;
        } else {
            LB=N;
        }
    }
    schedule_time = pack_vms(machines,UB);
    cout << "final rounds: " << UB << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
    //pack_vms(machines,UB);
}



void print_loads(const vector<vector<double> > &loads) {
    //cout << "Machine Loads:" << endl;
    for(vector<vector<double> >::const_iterator machine = loads.begin(); machine != loads.end(); ++machine) {
        cout << "  >";
        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            cout << " " << *vm;
        }
        cout << endl;
    }
    //cout << "End Machines" << endl;
}

bool DBPScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.empty() && round_schedules.empty()) {
        return false;
    } else if (!round_schedules.empty()) {
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    } else {
        schedule_vms(machines);
        machines.clear();
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    }
}

const char * DBPScheduler::getName() {
    return "Dual Bin Packing Scheduler 1 (ucow sort)";
}

double DBPScheduler2::pack_vms(vector<vector<double> > machines,int rounds) {
    round_schedules.clear();
    vector<vector<double> > machine_schedule; //empty machine schedule
    vector<double> vmschedule; //empty vm schedule
    //fill machine_schedule with p empty vectors (one for each machine)
    for(int i = 0; i < machines.size(); i++) {
        machine_schedule.push_back(vmschedule);
    }
    //add r empty schedules
    for(int i = 0; i < rounds; i++) {
        round_schedules.push_back(machine_schedule);
    }



    vector<double>::iterator vm;
    vector<vector<vector<double> > >::iterator round;
    int mid;
    while ((vm = pick_max_tdelta_vm(machines,mid)) != machines[0].end()) {
        //cout << "picked" << *vm << " on machine " << mid << endl;
        round = pick_min_newtime_round(round_schedules, machines, mid, vm);
        (*round)[mid].push_back(*vm);
        machines[mid].erase(vm);
    }

    double schedule_time = 0;
    for(vector<vector<vector<double> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        schedule_time += model_time(*round,false);
    }
    return schedule_time;
}

void DBPScheduler2::schedule_vms(vector<vector<double> > &machines) {
    int total_count = 0;
    int total_size = 0;
    double schedule_time;
    for(vector<vector<double> >::iterator machine = machines.begin();
            machine != machines.end(); ++machine) {
        for(vector<double>::iterator vm = (*machine).begin();
                vm != (*machine).end(); ++vm) {
            total_count++;
            total_size += *vm;
        }
    }
    cout << "total count: " << total_count << "; total machines: " << machines.size() << endl;
    int UB = 2*total_count/machines.size();
    if (UB > total_count) {
        UB = total_count;
    }
    int LB = 1;
    while (UB > LB) {
        int N=(UB+LB+1)/2;
        schedule_time = pack_vms(machines,N);
        cout << "rounds: " << N << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
        if (schedule_time > time_limit) {
            UB=N-1;
        } else {
            LB=N;
        }
    }
    schedule_time = pack_vms(machines,UB);
    cout << "final rounds: " << UB << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
}

bool DBPScheduler2::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.empty() && round_schedules.empty()) {
        return false;
    } else if (!round_schedules.empty()) {
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    } else {
        schedule_vms(machines);
        machines.clear();
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    }
}

const char * DBPScheduler2::getName() {
    return "Dual Bin Packing Scheduler 2 (tdelta sort)";
}

double DBPN1Scheduler::pack_vms(vector<vector<double> > machines,int rounds) {
    round_schedules.clear();
    vector<vector<double> > machine_schedule; //empty machine schedule
    vector<double> vmschedule; //empty vm schedule
    //fill machine_schedule with p empty vectors (one for each machine)
    for(int i = 0; i < machines.size(); i++) {
        machine_schedule.push_back(vmschedule);
    }
    //add r empty schedules
    for(int i = 0; i < rounds; i++) {
        round_schedules.push_back(machine_schedule);
    }



    vector<double>::iterator vm;
    vector<vector<vector<double> > >::iterator round;
    int mid;
    do {
        vm = pick_biggest_vm(machines,mid);
        if (vm == machines[0].end()) {
            break;
        }
        round = pick_min_time_round(round_schedules, machines, mid, vm);
        (*round)[mid].push_back(*vm);
        machines[mid].erase(vm);
    } while(true);

    double schedule_time = 0;
    for(vector<vector<vector<double> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        schedule_time += model_time(*round,false);
    }
    return schedule_time;
}

void DBPN1Scheduler::schedule_vms(vector<vector<double> > &machines) {
    int total_count = 0;
    int total_size = 0;
    double schedule_time;
    for(vector<vector<double> >::iterator machine = machines.begin();
            machine != machines.end(); ++machine) {
        for(vector<double>::iterator vm = (*machine).begin();
                vm != (*machine).end(); ++vm) {
            total_count++;
            total_size += *vm;
        }
    }
    cout << "total count: " << total_count << "; total machines: " << machines.size() << endl;
    int UB = 2*total_count/machines.size();
    if (UB > total_count) {
        UB = total_count;
    }
    int LB = 1;
    while (UB > LB) {
        int N=(UB+LB+1)/2;
        schedule_time = pack_vms(machines,N);
        cout << "rounds: " << N << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
        if (schedule_time > time_limit) {
            UB=N-1;
        } else {
            LB=N;
        }
    }
    schedule_time = pack_vms(machines,UB);
    cout << "final rounds: " << UB << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
}

bool DBPN1Scheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.empty() && round_schedules.empty()) {
        return false;
    } else if (!round_schedules.empty()) {
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    } else {
        schedule_vms(machines);
        machines.clear();
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    }
}

const char * DBPN1Scheduler::getName() {
    return "Naive Dual Bin Packing Scheduler 1";
}

double DBPN2Scheduler::pack_vms(vector<vector<double> > machines,int rounds) {
    round_schedules.clear();
    vector<vector<double> > machine_schedule; //empty machine schedule
    vector<double> vmschedule; //empty vm schedule
    //fill machine_schedule with p empty vectors (one for each machine)
    for(int i = 0; i < machines.size(); i++) {
        machine_schedule.push_back(vmschedule);
    }
    //add r empty schedules
    for(int i = 0; i < rounds; i++) {
        round_schedules.push_back(machine_schedule);
    }



    vector<double>::iterator vm;
    vector<vector<vector<double> > >::iterator round;
    int mid;
    do {
        vm = pick_biggest_vm(machines,mid);
        if (vm == machines[0].end()) {
            break;
        }
        round = pick_min_newtime_round(round_schedules, machines, mid, vm);
        (*round)[mid].push_back(*vm);
        machines[mid].erase(vm);
    } while(true);

    double schedule_time = 0;
    for(vector<vector<vector<double> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        schedule_time += model_time(*round,false);
    }
    return schedule_time;
}

void DBPN2Scheduler::schedule_vms(vector<vector<double> > &machines) {
    int total_count = 0;
    int total_size = 0;
    double schedule_time;
    for(vector<vector<double> >::iterator machine = machines.begin();
            machine != machines.end(); ++machine) {
        for(vector<double>::iterator vm = (*machine).begin();
                vm != (*machine).end(); ++vm) {
            total_count++;
            total_size += *vm;
        }
    }
    cout << "total count: " << total_count << "; total machines: " << machines.size() << endl;
    int UB = 2*total_count/machines.size();
    if (UB > total_count) {
        UB = total_count;
    }
    int LB = 1;
    while (UB > LB) {
        int N=(UB+LB+1)/2;
        schedule_time = pack_vms(machines,N);
        cout << "rounds: " << N << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
        if (schedule_time > time_limit) {
            UB=N-1;
        } else {
            LB=N;
        }
    }
    schedule_time = pack_vms(machines,UB);
    cout << "final rounds: " << UB << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
}

bool DBPN2Scheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
    if (machines.empty() && round_schedules.empty()) {
        return false;
    } else if (!round_schedules.empty()) {
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    } else {
        schedule_vms(machines);
        machines.clear();
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    }
}

const char * DBPN2Scheduler::getName() {
    return "Naive Dual Bin Packing Scheduler 2";
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
    //fill machine_schedule with p empty vectors (one for each machine)
    for(vector<vector<double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        vector<double> vmschedule;
        machine_schedule.push_back(vmschedule);
    }
    int total_machines = machines.size();
    //add r-1 empty schedules (the first schedule is filled with everything so is a special case)
    for(int i = 1; i < rounds; i++) {
        round_schedules.push_back(machine_schedule);
    }

    //count the vms so we can decide how many vms to move
    int total_vms = 0;
    for(vector<vector<double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine)
    {
        total_vms += (*machine).size();
    }

    //fill round 0 with all the vms initally (then we iteratively push vms forward)
    round_schedules[0].insert(round_schedules[0].end(), machines.begin(), machines.end());
    machines.clear();


    //in each iteration, we push vms from source_round to dest_round
    vector<vector<vector<double> > >::iterator source_round = round_schedules.begin();
    vector<vector<vector<double> > >::iterator dest_round = round_schedules.begin();
    ++dest_round; //dest round is always one ahead of source_round

    //loop through each additional round, pushing vms forward - this loop should maybe be based on some heuristic
    for(int r = 1; r < rounds; r++) {
        //to move right now performs an even split between rounds, should really be based on some heuristic
        //issue: there are some vms which should really be scheduled by themselves, even split prevents this (e.g. giant VM)
        int to_move = total_vms - r*(total_vms / rounds);

        //now we iteratively offload vms to the next round, maybe instead of a to_move loop there should be some target heuristic
        for(int i = 0; i < to_move; i++) {
            int mid;
            //first pick the vm to add (returns an iterator and machine id)
            //main metric is highest ucow (with tie-breaking based on machine load)
            vector<double>::iterator max_vm = pick_vm(*source_round, mid);
            //now add the vm to the correct machine in the dest_round
            (*dest_round)[mid].push_back(*max_vm);
            //finally remove the vm from the source_round
            (*source_round)[mid].erase(max_vm);
        }
        source_round=dest_round;
        ++dest_round;
    }


    round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
    round_schedules.pop_back();

    return true;
}

const char * CowScheduler::getName() {
    return "CoW Scheduler";
}
*/
