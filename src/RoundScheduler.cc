#include <vector>
#include <iostream>
#include <sstream>
#include "RoundScheduler.h"
//
//These globals define the actual parameters used in modeling
//double dirty_ratio = DEFAULT_DIRTY_RATIO; //block dirty ratio not used
double time_limit = DEFAULT_TIME_LIMIT;
double segment_dirty_ratio = DEFAULT_SEGMENT_DIRTY_RATIO;
double fp_ratio = DEFAULT_FP_RATIO;
double dupnew_ratio = DEFAULT_DUPNEW_RATIO; //dup-new ratio, as a fration of r
double net_latency = DEFAULT_NETWORK_LATENCY;
double read_bandwidth = DEFAULT_READ_BANDWIDTH;
double disk_write_bandwidth = DEFAULT_DISK_WRITE_BANDWIDTH;
//double backend_write_bandwidth = DEFAULT_BACKEND_WRITE_BANDWIDTH;
double lookup_time = DEFAULT_LOOKUP_TIME;
double network_memory = DEFAULT_NETWORK_MEMORY;
double write_rate = DEFAULT_WRITE_RATE;
double n = DEFAULT_MACHINE_INDEX_SIZE;
double u = DETECTION_REQUEST_SIZE;
double e = DUPLICATE_SUMMARY_SIZE;
double c = CHUNK_SIZE;

//formats time into raw seconds and hours/minutes/seconds
string format_time(double seconds) {
    stringstream ss;
    ss << seconds << " seconds (";
    int hours = 0;
    int minutes = 0;
    if (seconds > 3600) {
        hours = (int)(seconds / 3600);
        seconds -= 3600 * hours;
        ss << hours << "h";
    }
    if (seconds > 60) {
        minutes = (int)(seconds / 60);
        seconds -= 60 * minutes;
        ss << minutes << "m";
    }
    ss << seconds << "s)";
    return ss.str();
}

double model_time(double max_size, double total_size, int p, bool verbose) {
    double time_cost = 0;
    double b_r = read_bandwidth;
    double b_w = disk_write_bandwidth;
    //double b_b = backend_write_bandwidth;
    double m_n = network_memory;

    double r = (total_size * segment_dirty_ratio / c / p) * SIZE_UNIT; //avg num requests per machine
    double r_max = (max_size * segment_dirty_ratio / c) * SIZE_UNIT; //requests from most heavily loaded machine
    double r2_max = r_max * (1-fp_ratio); //number of new blocks at most heavily loaded machine; Not a real variable, but used often
    //cout << "r=" << r << "; r_max=" << r_max << endl;
    double t;

    if (verbose) {
        cout << "max size: " << max_size << endl <<
            "total size to backup: " << total_size << endl <<
            "size unit: " << SIZE_UNIT << 
            "p: " << p << endl <<
            "b_r: " << b_r << endl <<
            "b_w" << b_w << endl <<
            "r: " << r << "; r_max: " << r_max << endl <<
            "c: " << c << "; r_max: " << r_max << endl <<
            "net latency: " << net_latency << endl <<
            "u: " << u << endl;
    }

    //Stage 1a - exchange dirty data
    t = r_max * c / b_r; //read from disk
    t += net_latency * (u * r_max / m_n); //transfer dirty data
    t += u * r / b_w; //save requests to disk
    if (verbose) {
        cout << "    Stage 1a: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 1b - handle dedup requests
    t = r * u / b_r; //read requests from disk
    t += n*e/b_r; //read one machine's worth of index from disk
    t += r * lookup_time; //perform lookups
    t += r * e / b_w; //write out results of each lookup to one of 3 files per partition
    if (verbose) {
        cout << "    Stage 1b: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 2a - exchange new block results
    t = r * (1-fp_ratio) * e / b_r; //read new results from disk
    t += net_latency * (e * r2_max / m_n); //exchange new blocks
    t += r2_max * e / b_w; //save new block results to disk
    if (verbose) {
        cout << "    Stage 2a: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 2b - save new blocks
    t = r2_max * e / b_r; //read new results from disk
    t += r_max * c / b_r; //re-read dirty blocks
    //t += r2_max * c / b_b; //write new blocks to the backend - also update the lookup results
    if (verbose) {
        cout << "    Stage 2b: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 3a - exchange new block references
    t = r2_max * e / b_r; //re-read new block results from disk (now with references)
    t += net_latency * (e * r2_max / m_n); //exchange new block references back to parition holders
    t += r * (1-fp_ratio) * e / b_w; //save new block references at partition holder
    if (verbose) {
        cout << "    Stage 3a: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 3b - update index with new blocks and update dupnew results with references
    t = r * (1-fp_ratio) * e / b_r; //read new blocks for each partition from disk
    t += r * dupnew_ratio * e / b_r; //read dupnew results for each partition
    t += r * dupnew_ratio * lookup_time; //get references to the dupnew blocks
    t += r * dupnew_ratio * e / b_w; //add updated dup-new results to the list of dup results
    t += r * (1-fp_ratio) * e / b_w; //add new blocks to local partition index
    if (verbose) {
        cout << "    Stage 3b: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 4a - exchange dup (including dupnew) references
    t = r * fp_ratio * e / b_r; //read dup results from disk (including dupnew)
    t += net_latency * (e * r_max * fp_ratio / m_n); //exchange references to dup blocks
    t += r_max * fp_ratio * e / b_w; //save dup references to disk
    if (verbose) {
        cout << "    Stage 4a: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 4b - sort and save recipes back to storage service (we ignore this for now)

    return time_cost;

}

double measure_load(const vector<vector<double> > &machine_loads, double &max_size, int &max_mid) {
    max_size = 0;
    max_mid = 0;
    double total_size = 0;
    for(int i = 0; i < machine_loads.size(); i++) {
        double machine_size = 0;
        for(vector<double>::const_iterator vm = machine_loads[i].begin(); vm != machine_loads[i].end(); ++vm) {
            machine_size += (*vm);
        }
        if (machine_size > max_size) {
            max_size = machine_size;
            max_mid = i;
        }
        total_size += machine_size;
    }
    return total_size;
}

double measure_load(const vector<map<int, double> > &machine_loads, double &max_size, int &max_mid) {
    max_size = 0;
    max_mid = 0;
    double total_size = 0;
    for(int i = 0; i < machine_loads.size(); i++) {
        double machine_size = 0;
        for(map<int, double>::const_iterator vm = machine_loads[i].begin(); vm != machine_loads[i].end(); ++vm) {
            machine_size += vm->second;
        }
        if (machine_size > max_size) {
            max_size = machine_size;
            max_mid = i;
        }
        total_size += machine_size;
    }
    return total_size;
}

//uses the time model to compute how long a given round schedule would take, and optionally prints stage times
double model_time(const vector<map<int, double> > &machine_loads, bool verbose) {
    double max_size;
    int mid;
    double total_size = measure_load(machine_loads, max_size, mid);
    return model_time(max_size,total_size, machine_loads.size(),verbose);
}

//uses the time model to compute how long a given round schedule would take, and optionally prints stage times
//allows for modification of the load on one machine (adds load of size <vmsize> to machine <mid>
double model_time2(const vector<map<int, double> > &machine_loads, int mid, double vmsize, bool verbose) {
    double max_size = 0;
    double total_size = 0;
    for(int i = 0; i < machine_loads.size(); i++) {
        double machine_size = 0;
        if (i == mid) {
            machine_size += vmsize;
        }
        for(map<int, double>::const_iterator vm = machine_loads[i].begin(); vm != machine_loads[i].end(); ++vm) {
            machine_size += vm->second;
        }
        if (machine_size > max_size) {
            max_size = machine_size;
        }
        total_size += machine_size;
    }
    return model_time(max_size,total_size, machine_loads.size(),verbose);
}




void RoundScheduler::setMachineList(vector<map<int, double> > machine_loads) {
    machines=machine_loads;
}

void RoundScheduler::setTimeLimit(double seconds) {
    time_limit = seconds;
}


bool NullScheduler::schedule_round(std::vector<std::vector<int> > &round_schedule) {
    round_schedule.clear();
    if (machines.size() < 1) {
        return false;
    }
    bool schedule = false;
    for(vector<map<int,double> >::const_iterator machine = machines.begin(); machine != machines.end(); ++machine) {
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
        //for(int j = 0; j < machines[i].size(); j++) {
        //    machine_schedule.push_back(j);
        //}
        for(map<int,double>::iterator j = machines[i].begin(); j != machines[i].end(); ++j) {
            machine_schedule.push_back(j->first);
        }
        round_schedule.push_back(machine_schedule);
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
*/
//measures round duration after removing the specified vm from the specified machine
double model_rtime(const vector<map<int, double> > &machines, int mid, double vm) {
    return model_time2(machines,mid,-vm,false);
}
/*
//measures increase in round duration after adding the specified vm to the specified machine
double model_time_delta(const vector<vector<double> > &machines, int mid, int vm) {
    return model_time2(machines,mid,vm,false) - model_time(machines,false);
}
*/
//measures the time of the round after adding the specified vm to the specified machine
double model_new_time(const vector<map<int, double> > &machines, int mid, double vm) {
    return model_time2(machines,mid,vm,false);
}

//picks the vm whose removal will most decrease round time
//tie-breaker:machine with greatest load
map<int, double>::iterator pick_max_tdelta_vm(vector<map<int, double> > &machines, int &mid) {
    double max_size;
    int max_mid;
    double total_size = measure_load(machines, max_size,max_mid);
    double schedule_time = model_time(total_size, max_size, machines.size(), false);
    map<int, double>::iterator best_vm = machines[0].end();
    mid = -1;
    
    double best_time;
    double best_time_load;

    for(int i = 0; i < machines.size(); i++) {
        double machine_load = 0;
        for(map<int, double>::iterator vm = machines[i].begin(); vm != machines[i].end(); ++vm) {
            machine_load += vm->second;
        }
        for(map<int, double>::iterator vm = machines[i].begin(); vm != machines[i].end(); ++vm) {
            double rtime;
            //if we are removing from the max machine, we must recompute max_size
            if (max_mid == i) {
                rtime = model_rtime(machines, i, vm->second);
            } else {
                rtime = model_time(total_size - vm->second, max_size, machines.size() - 1, false);
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
/*
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
*/
//finds the round which will be the shortest in duration after adding the vm
vector<vector<vector<int> > >::iterator pick_min_newtime_round(vector<vector<vector<int> > > &round_schedules, const vector<map<int, double> > &machines, int mid, map<int,double>::const_iterator vm) {
    //double max_size;
    //int max_mid;
    //double total_size = measure_load(machines, max_size,max_mid);
    //double schedule_time = model_time(total_size, max_size, machines.size(), false);

    vector<vector<vector<int> > >::iterator best_round = round_schedules.end();
    double best_time;
    for(vector<vector<vector<int> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        vector<map<int,double> > loads;
        for(int i = 0; i < (*round).size(); i++) {
            map<int,double> machine_load;
            for(int j = 0; j < (*round)[i].size(); j++) {
                //stringstream ss;
                //ss << " indexing with " << i << " and " << ((*round)[i][j]) << endl;
                //cerr << ss.str();
                //int vid = ((*round)[i][j]);
                //machine_load[vid] = machines[i][vid];
                map<int,double>::const_iterator it = machines[i].find((*round)[i][j]);
                machine_load[(*round)[i][j]] = it->second;
            }
            loads.push_back(machine_load);
        }

        double time = model_new_time(loads, mid, vm->second);
        if (best_round == round_schedules.end() || time < best_time) {
            best_round = round;
            best_time = time;
        }
    }
    //cerr << "returning from pick_min_newtime" << endl;
    return best_round;
}
/*
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
*/


void print_loads(vector<map<int,double> > machines) {
    for(vector<map<int,double> >::iterator i = machines.begin(); i != machines.end(); ++i) {
        for (map<int, double>::iterator j = (*i).begin(); j != (*i).end(); ++j) {
            cout << "load[" << (i - machines.begin()) << "][" << j->first << "] = " << j->second << " ";
        }
        cout << endl;
    }
}

double DBPScheduler2::pack_vms(vector<map<int,double> > machines,int rounds) {
    //cout << "About to start packing vms into " << rounds << " rounds" << endl;
    round_schedules.clear();
    vector<vector<int> > machine_schedule; //empty machine schedule
    vector<int> vmschedule; //empty vm schedule
    //fill machine_schedule with p empty vectors (one for each machine)
    for(int i = 0; i < machines.size(); i++) {
        machine_schedule.push_back(vmschedule);
    }
    //add r empty schedules
    for(int i = 0; i < rounds; i++) {
        round_schedules.push_back(machine_schedule);
    }


    map<int, double>::iterator vm;
    vector<vector<vector<int> > >::iterator round;
    int mid;
    stringstream ss;
    while ((vm = pick_max_tdelta_vm(machines,mid)) != machines[0].end()) {
        //ss << "picked " << (vm - machines[mid].begin()) << "(" << *vm << " bytes) on machine " << mid << endl;
        //cerr << ss.str();
        //ss.str("");
        //ss.clear();
        //print_loads(machines);
        //cout << endl;
        round = pick_min_newtime_round(round_schedules, this->machines, mid, vm);
        //ss << "  picked round for vm" << endl;
        //cerr << ss.str();
        //ss.str("");
        //ss.clear();
        (*round)[mid].push_back(vm->first); //we push the index of the vm, not the size
        //cerr << "  pushed vm" << endl;
        //cout << "count was: " << machines[mid].size();
        machines[mid].erase(vm);
        //cout << "; picked [" << mid << "][" << vm->first << "]" << "; count is: " << machines[mid].size() << endl;
        //ss << "  erased vm" << endl;
        //cerr << ss.str();
        //ss.str("");
        //ss.clear();
    }
    //cerr << "finished scheduling vms" << endl;

    double schedule_time = 0;
    for(vector<vector<vector<int> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        vector<map<int, double> > loads;
        for(int i = 0; i < (*round).size(); i++) {
            map<int, double> machine_load;
            for(int j = 0; j < (*round)[i].size(); j++) {
                //cout << "accessing: round[" << i << "][" << j << "]" << endl;
                //cout << "accessing machines[" << i << "][" << (*round)[i][j] << "]" << endl;
                machine_load[(*round)[i][j]] = machines[i][((*round)[i][j])];
            }
            loads.push_back(machine_load);
        }
        schedule_time += model_time(loads,false);
    }
    cout << "Packed VMS to " << rounds << " rounds in " << format_time(schedule_time) << endl;
    return schedule_time;
}

void DBPScheduler2::schedule_vms(vector<map<int,double> > &machines) {
    //cout << "About to start scheduling vms" << endl;
    int total_count = 0;
    int total_size = 0;
    double schedule_time;
    for(vector<map<int, double> >::iterator machine = machines.begin();
            machine != machines.end(); ++machine) {
        for(map<int, double>::iterator vm = (*machine).begin();
                vm != (*machine).end(); ++vm) {
            total_count++;
            total_size += vm->second;
        }
    }
    //cout << "total count: " << total_count << "; total machines: " << machines.size() << endl;
    int UB = 2*total_count/machines.size();
    if (UB > total_count) {
        UB = total_count;
    }
    int LB = 1;
    while (UB > LB) {
        int N=(UB+LB+1)/2;
        schedule_time = pack_vms(machines,N);
        //cout << "rounds: " << N << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
        if (schedule_time > time_limit) {
            UB=N-1;
        } else {
            LB=N;
        }
    }
    schedule_time = pack_vms(machines,UB);
    stringstream ss;
    ss << "final rounds: " << UB << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
    cout << ss.str();
}

bool DBPScheduler2::schedule_round(std::vector<std::vector<int> > &round_schedule) {
    //cout << "About to start scheduling round" << endl;
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
/*
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
