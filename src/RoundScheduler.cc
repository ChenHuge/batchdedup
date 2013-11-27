#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <sstream>
#include "RoundScheduler.h"
//
//These globals define the actual parameters used in modeling
//double dirty_ratio = DEFAULT_DIRTY_RATIO; //block dirty ratio not used
//double time_limit = DEFAULT_TIME_LIMIT;
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

string print_settings(double time_limit) {
    stringstream ss;
    ss << "Current Settings:" << endl <<
        //"  dirty ratio: " << dirty_ratio << endl <<
        "  segment dirty ratio: " << segment_dirty_ratio << endl <<
        "  fp ratio: " << fp_ratio << endl <<
        "  dupnew ratio: " << dupnew_ratio << endl <<
        "  network latency: " << net_latency << " seconds" << endl <<
        "  disk read bandwidth: " << read_bandwidth << " B/s" << endl <<
        "  disk write bandwidth: " << disk_write_bandwidth << " B/s" << endl <<
        //"  backend write bandwidth: " << backend_write_bandwidth << " MB/s" << endl <<
        "  fp lookup time: " << lookup_time << " seconds" << endl <<
        "  network memory usage: " << network_memory << " B" << endl <<
        "  machine index size: " << n << " entries" << endl <<
        "  time limit: " << format_time(time_limit) << endl <<
        "  c=" << c << "; u=" << u << "; e=" << e << endl;
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
    stringstream ss;

    if (verbose) {
        ss << "max size: " << max_size << endl <<
            "total size to backup: " << total_size << "; size unit: " << SIZE_UNIT << endl <<
            "p: " << p << endl <<
            "b_r: " << b_r << "; b_w: " << b_w << endl <<
            "r: " << r << "; r_max: " << r_max << endl <<
            "c: " << c << endl <<
            "net latency: " << net_latency << endl <<
            "u: " << u << endl;
    }

    //Stage 1a - exchange dirty data
    t = r_max * c / b_r; //read from disk
    t += net_latency * (u * r_max / m_n); //transfer dirty data
    t += u * r / b_w; //save requests to disk
    if (verbose) {
        ss << "    Stage 1a: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 1b - handle dedup requests
    t = r * u / b_r; //read requests from disk
    t += n*e/b_r; //read one machine's worth of index from disk
    t += r * lookup_time; //perform lookups
    t += r * e / b_w; //write out results of each lookup to one of 3 files per partition
    if (verbose) {
        ss << "    Stage 1b: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 2a - exchange new block results
    t = r * (1-fp_ratio) * e / b_r; //read new results from disk
    t += net_latency * (e * r2_max / m_n); //exchange new blocks
    t += r2_max * e / b_w; //save new block results to disk
    if (verbose) {
        ss << "    Stage 2a: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 2b - save new blocks
    t = r2_max * e / b_r; //read new results from disk
    t += r_max * c / b_r; //re-read dirty blocks
    //t += r2_max * c / b_b; //write new blocks to the backend - also update the lookup results
    if (verbose) {
        ss << "    Stage 2b: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 3a - exchange new block references
    t = r2_max * e / b_r; //re-read new block results from disk (now with references)
    t += net_latency * (e * r2_max / m_n); //exchange new block references back to parition holders
    t += r * (1-fp_ratio) * e / b_w; //save new block references at partition holder
    if (verbose) {
        ss << "    Stage 3a: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 3b - update index with new blocks and update dupnew results with references
    t = r * (1-fp_ratio) * e / b_r; //read new blocks for each partition from disk
    t += r * dupnew_ratio * e / b_r; //read dupnew results for each partition
    t += r * dupnew_ratio * lookup_time; //get references to the dupnew blocks
    t += r * dupnew_ratio * e / b_w; //add updated dup-new results to the list of dup results
    t += r * (1-fp_ratio) * e / b_w; //add new blocks to local partition index
    if (verbose) {
        ss << "    Stage 3b: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 4a - exchange dup (including dupnew) references
    t = r * fp_ratio * e / b_r; //read dup results from disk (including dupnew)
    t += net_latency * (e * r_max * fp_ratio / m_n); //exchange references to dup blocks
    t += r_max * fp_ratio * e / b_w; //save dup references to disk
    if (verbose) {
        ss << "    Stage 4a: " << format_time(t) << endl;
    }
    time_cost += t;
    //Stage 4b - sort and save recipes back to storage service (we ignore this for now)
    if (verbose)
        cout << ss.str();

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
        //cout << "Round Load:";
        for(int i = 0; i < (*round).size(); i++) {
            //cout << " { ";
            map<int,double> machine_load;
            for(int j = 0; j < (*round)[i].size(); j++) {
                map<int,double>::const_iterator it = machines[i].find((*round)[i][j]);
                //cout << it->first << "->" << it->second << " ";
                machine_load[(*round)[i][j]] = it->second;
            }
            //cout << "}";
            loads.push_back(machine_load);
        }
        //cout << endl;

        double time = model_new_time(loads, mid, vm->second);
        if (best_round == round_schedules.end() || time < best_time) {
            best_round = round;
            best_time = time;
        }
    }
    //cerr << "returning from pick_min_newtime" << endl;
    return best_round;
}

string print_loads(vector<map<int,double> > machines) {
    stringstream ss;
    for(vector<map<int,double> >::iterator i = machines.begin(); i != machines.end(); ++i) {
        ss.str("");
        ss.clear();
        ss << "Machine " << (i - machines.begin()) << " vms:";
        for (map<int, double>::iterator j = (*i).begin(); j != (*i).end(); ++j) {
            ss << " " << j->first << "->" << j->second;
        }
        ss << endl;
    }
    return ss.str();
}

string print_loads(vector<vector<vector<int> > > round_schedules, vector<map<int,double> > machines) {
    stringstream ss;
    ss << "Loads: " << endl;
    for(vector<vector<vector<int> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        ss << "Round Load:";
        for(int i = 0; i < (*round).size(); i++) {
            ss << " { ";
            for(int j = 0; j < (*round)[i].size(); j++) {
                map<int,double>::const_iterator it = machines[i].find((*round)[i][j]);
                ss << it->first << "->" << it->second << " ";
            }
            ss << "}";
        }
        ss << endl;
    }
    return ss.str();
}

string print_schedules(vector<vector<vector<int> > > round_schedules) {
    stringstream ss;
    ss << "Schedules: " << endl;
    for(vector<vector<vector<int> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        ss << "Round Load:";
        for(int i = 0; i < (*round).size(); i++) {
            ss << " { ";
            for(int j = 0; j < (*round)[i].size(); j++) {
                ss << (*round)[i][j] << " ";
            }
            ss << "}";
        }
        ss << endl;
    }
    return ss.str();
}

double DBPScheduler2::pack_vms(vector<map<int,double> > machines,int rounds) {
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
        round = pick_min_newtime_round(round_schedules, this->machines, mid, vm);
        (*round)[mid].push_back(vm->first); //we push the index of the vm, not the size
        machines[mid].erase(vm);
    }

    double schedule_time = 0;
    for(vector<vector<vector<int> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        vector<map<int, double> > loads;
        for(int i = 0; i < (*round).size(); i++) {
            map<int, double> machine_load;
            for(int j = 0; j < (*round)[i].size(); j++) {
                //ss << "  machines[" << i << "][" << (*round)[i][j] << "] = ";
                map<int,double>::const_iterator lookup = this->machines[i].find((*round)[i][j]);
                if (lookup == this->machines[i].end()) {
                    cout << "  machines[" << i << "][" << (*round)[i][j] << "] = NOT FOUND";
                } else {
                    machine_load[(*round)[i][j]] = lookup->second;
                }
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
    double total_size = 0;
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
    ss << "final rounds: " << UB << "; time: " << schedule_time << "; time limit: " << time_limit << "; count: " << total_count << "; data: " << total_size << endl;
    cout << ss.str();
}

DBPScheduler2::DBPScheduler2() {
    scheduled = false;
}

bool DBPScheduler2::schedule_round(std::vector<std::vector<int> > &round_schedule) {
    //cout << "About to start scheduling round" << endl;
    round_schedule.clear();
    if (scheduled && round_schedules.empty()) {
        return false; //finished with schedule
    } else if (!round_schedules.empty()) {
        //have rounds to schedule
        //cout << print_schedules(round_schedules);
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    } else if (machines.empty()) {
        //we haven't been scheduled yet, but there are no machines to schedule
        return false;
    } else { //we need to schedule
        schedule_vms(machines);
        scheduled = true;
        //cout << print_loads(round_schedules, machines);
        //cout << print_schedules(round_schedules);
        //cout << format_time(model_time(machines,true));
        machines.clear(); //TODO: is this a problem?
        //cerr << print_settings(time_limit);
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        //cout << print_schedules(round_schedules);
        return true;
    }
}

const char * DBPScheduler2::getName() {
    return "Dual Bin Packing Scheduler 2 (tdelta sort)";
}

bool valcompare (pair<int,double> a,pair<int,double> b) { return (a.second < b.second); }

double BPLScheduler::pack_vms(vector<map<int,double> > machines,int rounds) {
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

    //loop through the machines
    for(vector<map<int,double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        vector<pair<int,double> > vmlist((*machine).begin(), (*machine).end());
        std::sort(vmlist.begin(), vmlist.end());
        int mid = machine - machines.begin();
        //loop through the descending order sorted vms for this machine
        for(vector<pair<int,double> >::iterator vm = vmlist.begin(); vm != vmlist.end(); ++vm) {
            vector<vector<vector<int> > >::iterator min_round = round_schedules.end();
            double min_round_load;
            //find the least loaded round for this machine
            for(vector<vector<vector<int> > >::iterator rload = round_schedules.begin(); rload != round_schedules.end(); ++rload) {
                double round_load = 0;
                //for(vector<vector<double> >::cost_iterator mload = (*rload).begin(); mload != (*rload).end(); ++mload) {
                for(vector<int>::const_iterator vload = (*rload)[mid].begin(); vload != (*rload)[mid].end(); ++vload) {
                    round_load += machines[mid][*vload];
                }
                //}
                if (min_round == round_schedules.end() || round_load < min_round_load) {
                    min_round_load = round_load;
                    min_round = rload;
                }
            }
            //schedule this vm to the least loaded round for this machine
            (*min_round)[mid].push_back(vm->first);
        }
    }

    double schedule_time = 0;
    for(vector<vector<vector<int> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        vector<map<int, double> > loads;
        for(int i = 0; i < (*round).size(); i++) {
            map<int, double> machine_load;
            for(int j = 0; j < (*round)[i].size(); j++) {
                //ss << "  machines[" << i << "][" << (*round)[i][j] << "] = ";
                map<int,double>::const_iterator lookup = this->machines[i].find((*round)[i][j]);
                if (lookup == this->machines[i].end()) {
                    cout << "  machines[" << i << "][" << (*round)[i][j] << "] = NOT FOUND";
                } else {
                    machine_load[(*round)[i][j]] = lookup->second;
                }
            }
            loads.push_back(machine_load);
        }
        schedule_time += model_time(loads,false);
    }
    cout << "Packed VMS to " << rounds << " rounds in " << format_time(schedule_time) << endl;
    return schedule_time;
}

void BPLScheduler::schedule_vms(vector<map<int,double> > &machines) {
    //cout << "About to start scheduling vms" << endl;
    int total_count = 0;
    double total_size = 0;
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
    ss << "final rounds: " << UB << "; time: " << schedule_time << "; time limit: " << time_limit << "; count: " << total_count << "; data: " << total_size << endl;
    cout << ss.str();
}

BPLScheduler::BPLScheduler() {
    scheduled = false;
}

bool BPLScheduler::schedule_round(std::vector<std::vector<int> > &round_schedule) {
    //cout << "About to start scheduling round" << endl;
    round_schedule.clear();
    if (scheduled && round_schedules.empty()) {
        return false; //finished with schedule
    } else if (!round_schedules.empty()) {
        //have rounds to schedule
        //cout << print_schedules(round_schedules);
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    } else if (machines.empty()) {
        //we haven't been scheduled yet, but there are no machines to schedule
        return false;
    } else { //we need to schedule
        schedule_vms(machines);
        scheduled = true;
        //cout << print_loads(round_schedules, machines);
        //cout << print_schedules(round_schedules);
        //cout << format_time(model_time(machines,true));
        machines.clear(); //TODO: is this a problem?
        //cerr << print_settings(time_limit);
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        //cout << print_schedules(round_schedules);
        return true;
    }
}

const char * BPLScheduler::getName() {
    return "Bin Packing Scheduler w/bin-search";
}

double BPLScheduler2::pack_vms(vector<map<int,double> > machines,int rounds) {
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

    //loop through the machines
    for(vector<map<int,double> >::iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        vector<pair<int,double> > vmlist((*machine).begin(), (*machine).end());
        std::sort(vmlist.begin(), vmlist.end());
        int mid = machine - machines.begin();
        //loop through the descending order sorted vms for this machine
        for(vector<pair<int,double> >::iterator vm = vmlist.begin(); vm != vmlist.end(); ++vm) {
            vector<vector<vector<int> > >::iterator min_round = round_schedules.end();
            double min_round_load;
            //find the least loaded round for this machine
            for(vector<vector<vector<int> > >::iterator rload = round_schedules.begin(); rload != round_schedules.end(); ++rload) {
                double round_load = 0;
                //for(vector<vector<double> >::cost_iterator mload = (*rload).begin(); mload != (*rload).end(); ++mload) {
                for(vector<int>::const_iterator vload = (*rload)[mid].begin(); vload != (*rload)[mid].end(); ++vload) {
                    round_load += machines[mid][*vload];
                }
                //}
                if (min_round == round_schedules.end() || round_load < min_round_load) {
                    min_round_load = round_load;
                    min_round = rload;
                }
            }
            //schedule this vm to the least loaded round for this machine
            (*min_round)[mid].push_back(vm->first);
        }
    }

    double schedule_time = 0;
    for(vector<vector<vector<int> > >::iterator round = round_schedules.begin();
            round != round_schedules.end(); ++round) {
        vector<map<int, double> > loads;
        for(int i = 0; i < (*round).size(); i++) {
            map<int, double> machine_load;
            for(int j = 0; j < (*round)[i].size(); j++) {
                //ss << "  machines[" << i << "][" << (*round)[i][j] << "] = ";
                map<int,double>::const_iterator lookup = this->machines[i].find((*round)[i][j]);
                if (lookup == this->machines[i].end()) {
                    cout << "  machines[" << i << "][" << (*round)[i][j] << "] = NOT FOUND";
                } else {
                    machine_load[(*round)[i][j]] = lookup->second;
                }
            }
            loads.push_back(machine_load);
        }
        schedule_time += model_time(loads,false);
    }
    cout << "Packed VMS to " << rounds << " rounds in " << format_time(schedule_time) << endl;
    return schedule_time;
}

double bpl2cost(double alpha, double jobspan, double minjobspan,double avgbackup, double minavgbackup) {
    cout << "cost: " << alpha << " * " << jobspan << "/" << minavgbackup << "  +  " << (1-alpha) << " * " << jobspan << "/" << minjobspan << endl;
    return alpha*(avgbackup/minavgbackup) + (1-alpha)*(jobspan/minjobspan);
}

double getminavgbackup(vector<map<int,double> > &machines) {
    double time = 0;
    int vm_count;
    for(vector<map<int, double> >::const_iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        for(map<int,double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            time += model_time(vm->second, vm->second, 1, false);
            vm_count++;
        }
    }
    return time / vm_count;
}

double getavgbackup(vector<map<int,double> > &machines, vector<vector<vector<int> > > &schedule) {
    double schedule_time = 0;
    int total_vms = 0;
    for(vector<vector<vector<int> > >::iterator round = schedule.begin();
            round != schedule.end(); ++round) {
        vector<map<int, double> > loads;
        int round_vms = 0;
        for(int i = 0; i < (*round).size(); i++) {
            map<int, double> machine_load;
            for(int j = 0; j < (*round)[i].size(); j++) {
                //ss << "  machines[" << i << "][" << (*round)[i][j] << "] = ";
                map<int,double>::const_iterator lookup = machines[i].find((*round)[i][j]);
                if (lookup == machines[i].end()) {
                    cout << "  machines[" << i << "][" << (*round)[i][j] << "] = NOT FOUND";
                } else {
                    machine_load[(*round)[i][j]] = lookup->second;
                    round_vms++;
                }
            }
            loads.push_back(machine_load);
        }
        total_vms += round_vms;
        schedule_time += model_time(loads,false) * round_vms;
    }
    return schedule_time / total_vms;
}

double getminjobspan(vector<map<int,double> > &machines) {
    return model_time(machines, false);
}

void BPLScheduler2::schedule_vms(vector<map<int,double> > &machines) {
    //cout << "About to start scheduling vms" << endl;
    double minjobspan = getminjobspan(machines);
    double minavgbackup = getminavgbackup(machines);
    int total_count = 0;
    double total_size = 0;
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
    int k = 1;
    int mink = 1;
    double mincost;
    while (k < UB) {
        schedule_time = pack_vms(machines,k);
        double cost = bpl2cost(alpha, schedule_time, minjobspan, getavgbackup(machines,round_schedules), minavgbackup);
        //cout << "rounds: " << k << "; time: " << schedule_time << "; time limit: " << time_limit << endl;
        if (schedule_time > time_limit) {
            break;
        } else {
            if (k == 1 || cost < mincost) {
                mink = k;
                mincost = cost;
            }
        }
        k++;
    }
    schedule_time = pack_vms(machines,mink);
    stringstream ss;
    ss << "final rounds: " << mink << "; time: " << schedule_time << "; time limit: " << time_limit << "; count: " << total_count << "; data: " << total_size << endl;
    cout << ss.str();
}

BPLScheduler2::BPLScheduler2() {
    scheduled = false;
    alpha = 0.5;
}

BPLScheduler2::BPLScheduler2(double alpha) {
    scheduled = false;
    this->alpha = alpha;
}

bool BPLScheduler2::schedule_round(std::vector<std::vector<int> > &round_schedule) {
    //cout << "About to start scheduling round" << endl;
    round_schedule.clear();
    if (scheduled && round_schedules.empty()) {
        return false; //finished with schedule
    } else if (!round_schedules.empty()) {
        //have rounds to schedule
        //cout << print_schedules(round_schedules);
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        return true;
    } else if (machines.empty()) {
        //we haven't been scheduled yet, but there are no machines to schedule
        return false;
    } else { //we need to schedule
        schedule_vms(machines);
        scheduled = true;
        //cout << print_loads(round_schedules, machines);
        //cout << print_schedules(round_schedules);
        //cout << format_time(model_time(machines,true));
        machines.clear(); //TODO: is this a problem?
        //cerr << print_settings(time_limit);
        round_schedule.insert(round_schedule.end(),round_schedules.back().begin(), round_schedules.back().end());
        round_schedules.pop_back();
        //cout << print_schedules(round_schedules);
        return true;
    }
}

const char * BPLScheduler2::getName() {
    return "Bin Packing Scheduler w/cost minimization";
}
