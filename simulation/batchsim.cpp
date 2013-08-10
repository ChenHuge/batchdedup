#include <iostream>
#include <fstream>
#include <vector>
#include <cstring> //strcmp
#include <string>
#include "BackupScheduler.h"
#include <cstdlib> //strtod

using namespace std;
double schedule_round(vector<vector<double> > &machine_loads, vector<vector<double> > &round_schedule);
double model_time(const vector<vector<double> > &machine_loads);
double model_cow(const vector<vector<double> > &machine_loads);

#define DEFAULT_DIRTY_PERCENT 0.12
#define DEFAULT_NETWORK_LATENCY 0.0005
#define DEFAULT_DISK_BANDWIDTH (30.0*(1<<20))

double dirty_percent = DEFAULT_DIRTY_PERCENT;
double net_latency = DEFAULT_NETWORK_LATENCY;
double disk_bandwidth = DEFAULT_DISK_BANDWIDTH;

int main (int argc, char *argv[]) {
    int argi = 1;
    const char* machinefile = NULL;
    char *endptr;
    vector<BackupScheduler*> schedulers;
    while(argi < argc && argv[argi][0] == '-') {
        if (!strcmp(argv[argi],"--")) {
            argi++; break;
        } else if (!strcmp(argv[argi],"--machinefile") || !strcmp(argv[argi],"-m")) {
            machinefile=argv[++argi];
            argi++;
        } else if (!strcmp(argv[argi],"--allschedule")) {
            argi++;
            schedulers.push_back(new AllScheduler());
        } else if (!strcmp(argv[argi],"--dirty")) {
            dirty_percent = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid dirty ratio or none given";
                return -1;
            }
        } else if (!strcmp(argv[argi],"--netlatency")) {
            net_latency = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid network latency or none given";
                return -1;
            }
        } else if (!strcmp(argv[argi],"--diskbandwidth")) {
            disk_bandwidth = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid disk bandwidth or none given";
                return -1;
            }
        } else {
            cout << "Unknown argument: " << argv[argi] << endl;
            return -1;
        }
    }

    if (machinefile == NULL) {
        cout << "must specify machine file" << endl;
        return -1;
    }

    vector<vector<double> > machinelist;

    ifstream machine_list_stream;
    ifstream machine_vm_stream;
    string line;
    machine_list_stream.open(machinefile);
    if (!machine_list_stream.is_open()) {
        cout << "could not open machine file (" << machinefile << ")" << endl;
        return -1;
    }

    while(getline(machine_list_stream,line)) {
        if (line.length() < 1 || line[0] == '\n') {
            continue;
        }
        machine_vm_stream.open(line.c_str());
        if (!machine_vm_stream.is_open()) {
            cout << "could not open machine vm list (" << line << ")" << endl;
            return -1;
        }
        vector<double> vmlist;
        double size;
        while(machine_vm_stream >> size) {
            vmlist.push_back(size);
        }
        machine_vm_stream.close();
        machinelist.push_back(vmlist);
    }
    machine_list_stream.close();

    //for(int i = 0; i < machinelist.size(); i++) {
    //    for(int j = 0; j < machinelist[i].size(); j++) {
    //        cout << machinelist[i][j] << endl;
    //    }
    //}

    for(std::vector<BackupScheduler*>::iterator scheduler = schedulers.begin(); scheduler != schedulers.end(); ++scheduler) {
        int round = 1;
        double total_time = 0;
        double total_cow = 0;
        vector<vector<double> > schedule;
        (*scheduler)->setMachineList(machinelist);
        cout << "Scheduler: " << (*scheduler)->getName() << endl;
        while((*scheduler)->schedule_round(schedule)) {
            double round_time = model_time(schedule);
            double round_cow = model_cow(schedule);
            cout << "  Round " << round << ": Round Time=" << round_time << 
                "; Round CoW: " << round_cow << endl;
            total_time += round_time;
            total_cow += round_cow;
            schedule.clear();
        }
        cout << "  Total Time: " << total_time << 
            "; CoW Cost: " << total_cow << endl;
    }
    for(std::vector<BackupScheduler*>::iterator it = schedulers.begin(); it != schedulers.end(); ++it) {
        delete *it;
    }
    schedulers.clear();
}

double model_time(const vector<vector<double> > &machine_loads) {
    double max_cost = 0;
    for(vector<vector<double> >::const_iterator machine = machine_loads.begin(); machine != machine_loads.end(); ++machine) {
        double machine_cost = 0;
        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            machine_cost += (*vm);
        }
        if (machine_cost > max_cost) {
            max_cost = machine_cost;
        }
    }
    return max_cost;
}

double model_cow(const vector<vector<double> > &machine_loads) {
    double biggest_machine = 0;
    for(vector<vector<double> >::const_iterator machine = machine_loads.begin(); machine != machine_loads.end(); ++machine) {
        double machine_cost = 0;
        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            machine_cost += (*vm);
        }
        if (machine_cost > biggest_machine) {
            biggest_machine = machine_cost;
        }
    }

    for(vector<vector<double> >::const_iterator machine = machine_loads.begin(); machine != machine_loads.end(); ++machine) {
        double machine_cost = 0;
        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            machine_cost += (*vm);
        }
        //insert CoW cost analysis here
    }

    return biggest_machine;
}
