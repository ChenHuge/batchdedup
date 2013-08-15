#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cstring> //strcmp
#include <string>
#include "BackupScheduler.h"
#include <cstdlib> //strtod

using namespace std;
double schedule_round(vector<vector<double> > &machine_loads, vector<vector<double> > &round_schedule);
double model_time(const vector<vector<double> > &machine_loads);
double model_cow(const vector<vector<double> > &machine_loads);
string format_time(double seconds);

#define DEFAULT_DIRTY_RATIO 0.012
#define DEFAULT_FP_RATIO 0.5
#define DEFAULT_DUPNEW_RATIO 0.01
#define DEFAULT_NETWORK_LATENCY 0.0005
#define DEFAULT_READ_BANDWIDTH 30.0
#define DEFAULT_DISK_WRITE_BANDWIDTH 30.0
#define DEFAULT_BACKEND_WRITE_BANDWIDTH 30.0
#define DETECTION_REQUEST_SIZE 40.0
#define DUPLICATE_SUMMARY_SIZE 48.0
#define CHUNK_SIZE 4096.0
#define SIZE_UNIT (1<<30)
#define MEMORY_UNIT (1<<20)
#define BANDWIDTH_UNIT (1<<20)
#define DEFAULT_LOOKUP_TIME 0.000001
#define DEFAULT_MACHINE_INDEX_SIZE (1024.0 / CHUNK_SIZE * (1<<30))
#define DEFAULT_NETWORK_MEMORY 25.0

double dirty_ratio = DEFAULT_DIRTY_RATIO;
double fp_ratio = DEFAULT_FP_RATIO;
double dupnew_ratio = DEFAULT_DUPNEW_RATIO; //dup-new ratio, as a fration of r
double net_latency = DEFAULT_NETWORK_LATENCY;
double read_bandwidth = DEFAULT_READ_BANDWIDTH;
double disk_write_bandwidth = DEFAULT_DISK_WRITE_BANDWIDTH;
double backend_write_bandwidth = DEFAULT_BACKEND_WRITE_BANDWIDTH;
double lookup_time = DEFAULT_LOOKUP_TIME;
double network_memory = DEFAULT_NETWORK_MEMORY;
double n = DEFAULT_MACHINE_INDEX_SIZE;
double u = DETECTION_REQUEST_SIZE;
double e = DUPLICATE_SUMMARY_SIZE;
double c = CHUNK_SIZE;

void usage(const char* progname) {
    cout << "Usage: " << progname << " <parameters>" << endl <<
        "Where <parameters> can be:" << endl <<
        "  --machinefile <path> - file containing list of files with machine loads" << endl <<
        "                       - each machine load should be a size in GB" << endl << 
        "  --dirty <ratio> - default=" << DEFAULT_DIRTY_RATIO << endl <<
        "  --netlatency <seconds> - default=" << DEFAULT_NETWORK_LATENCY << endl <<
        "  --readbandwidth <MB/s> - default=" << DEFAULT_READ_BANDWIDTH << endl <<
        "  --diskwritebandwidth <MB/s> - default=" << DEFAULT_DISK_WRITE_BANDWIDTH << endl <<
        "  --backwritebandwidth <MB/s> - default=" << DEFAULT_BACKEND_WRITE_BANDWIDTH << endl <<
        "  --indexsize <machine_index_entries> - default=" << DEFAULT_MACHINE_INDEX_SIZE << endl <<
        "Schedule Types:" << endl <<
        "  --nullschedule - just schedule all the jobs at once" << endl;
}

void print_settings() {
    cout << "Current Settings:" << endl <<
        "  dirty ratio: " << dirty_ratio << endl <<
        "  fp ratio: " << fp_ratio << endl <<
        "  dupnew ratio: " << dupnew_ratio << endl <<
        "  network latency: " << net_latency << " seconds" << endl <<
        "  disk read bandwidth: " << read_bandwidth << " MB/s" << endl <<
        "  disk write bandwidth: " << disk_write_bandwidth << " MB/s" << endl <<
        "  backend write bandwidth: " << backend_write_bandwidth << " MB/s" << endl <<
        "  fp lookup time: " << lookup_time << " seconds" << endl <<
        "  network memory usage: " << network_memory << " MB" << endl <<
        "  machine index size: " << n << " entries" << endl <<
        "  c=" << c << "; u=" << u << "; e=" << e << endl;
}

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
        } else if (!strcmp(argv[argi],"--nullschedule")) {
            argi++;
            schedulers.push_back(new NullScheduler());
        } else if (!strcmp(argv[argi],"--dirty")) {
            dirty_ratio = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid dirty ratio or none given";
                usage(argv[0]);
                return -1;
            }
        } else if (!strcmp(argv[argi],"--netlatency")) {
            net_latency = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid network latency or none given";
                usage(argv[0]);
                return -1;
            }
        } else if (!strcmp(argv[argi],"--readbandwidth")) {
            read_bandwidth = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid disk bandwidth or none given";
                usage(argv[0]);
                return -1;
            }
        } else if (!strcmp(argv[argi],"--diskwritebandwidth")) {
            disk_write_bandwidth = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid disk bandwidth or none given";
                usage(argv[0]);
                return -1;
            }
        } else if (!strcmp(argv[argi],"--backwritebandwidth")) {
            backend_write_bandwidth = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid disk bandwidth or none given";
                usage(argv[0]);
                return -1;
            }
        } else if (!strcmp(argv[argi],"--indexsize")) {
            n = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid machine index size or none given";
                usage(argv[0]);
                return -1;
            }
        } else {
            cout << "Unknown argument: " << argv[argi] << endl;
            usage(argv[0]);
            return -1;
        }
    }

    if (machinefile == NULL) {
        cout << "must specify machine file" << endl;
        usage(argv[0]);
        return -1;
    }

    print_settings();
    
    //convert MB/s to B/s
    read_bandwidth *= BANDWIDTH_UNIT;
    disk_write_bandwidth *= BANDWIDTH_UNIT;
    backend_write_bandwidth *= BANDWIDTH_UNIT;
    
    //convert memory unit (MB) to bytes
    network_memory *= MEMORY_UNIT;
    print_settings();

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
            cout << "  Round " << round << ": Round Time=" << format_time(round_time) << 
                "; Round CoW: " << round_cow << endl;
            total_time += round_time;
            total_cow += round_cow;
            schedule.clear();
        }
        cout << "  Total Time: " << format_time(total_time) << 
            "; CoW Cost: " << total_cow << endl;
    }
    for(std::vector<BackupScheduler*>::iterator it = schedulers.begin(); it != schedulers.end(); ++it) {
        delete *it;
    }
    schedulers.clear();
}

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

double model_time(const vector<vector<double> > &machine_loads) {
    double max_cost = 0;
    double total_size = 0;
    double time_cost = 0;
    double p = machine_loads.size();
    double b_d = read_bandwidth;
    double b_wd = disk_write_bandwidth;
    double b_wb = backend_write_bandwidth;
    double m_n = network_memory;
    for(vector<vector<double> >::const_iterator machine = machine_loads.begin(); machine != machine_loads.end(); ++machine) {
        double machine_cost = 0;
        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            machine_cost += (*vm);
        }
        if (machine_cost > max_cost) {
            max_cost = machine_cost;
        }
        total_size += machine_cost;
    }
    double r = (total_size * dirty_ratio / c) * SIZE_UNIT; //avg num requests per machine
    double r_max = (max_cost / c) * SIZE_UNIT; //requests from most heavily loaded machine
    double r2_max = r_max * (1-fp_ratio); //number of new blocks at most heavily loaded machine; Not a real variable, but used often
    //cout << "r_max=" << r_max << endl;

    //Stage 1a - exchange dirty data
    time_cost += r_max * c / b_d; //read from disk
    time_cost += net_latency * (u * r_max * p / m_n); //transfer dirty data
    time_cost += u * r / b_wd; //save requests to disk
    //Stage 1b - handle dedup requests
    time_cost += r * u / b_d; //read requests from disk
    time_cost += n*e/b_d; //read one machine's worth of index from disk
    time_cost += r * lookup_time; //perform lookups
    time_cost += r * e / b_d; //write out results of each lookup to one of 3 files per partition
    //Stage 2a - exchange new block results
    time_cost += r * (1-fp_ratio) * e / b_d; //read new results from disk
    time_cost += net_latency * (e * r2_max * p / m_n); //exchange new blocks
    time_cost += r2_max * e / b_wd; //save new block results to disk
    //Stage 2b - save new blocks
    time_cost += r2_max * (e+c) / b_d; //read new block results from disk, and the associated blocks
    time_cost += r2_max * c / b_wb; //write new blocks to the backend - also update the lookup results
    //Stage 3a - exchange new block references
    time_cost += r2_max * e / b_wd; //re-read new block results from disk (now with references)
    time_cost += net_latency * (e * r2_max * p / m_n); //exchange new block references back to parition holders
    time_cost += r * (1-fp_ratio) * e / b_wd; //save new block references at partition holder
    //Stage 3b - update index with new blocks and update dupnew results with references
    time_cost += r * (1-fp_ratio) * e / b_d; //read new blocks for each partition from disk
    time_cost += r * dupnew_ratio * e / b_d; //read dupnew results for each partition
    time_cost += r * dupnew_ratio * lookup_time; //get references to the dupnew blocks
    time_cost += r * dupnew_ratio * e / b_wd; //add updated dup-new results to the list of dup results
    time_cost += r * (1-fp_ratio) * e / b_wd; //add new blocks to local partition index
    //Stage 4a - exchange dup (including dupnew) references
    time_cost += r * fp_ratio * e / b_d; //read dup results from disk (including dupnew)
    time_cost += net_latency * (e * r_max * fp_ratio * p / m_n); //exchange references to dup blocks
    time_cost += r_max * fp_ratio * e / b_wd; //save dup references to disk
    //Stage 4b - sort and save recipes back to storage service (we ignore this for now)

    return time_cost;
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
