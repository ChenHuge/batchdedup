#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib> //strtod
#include <cstring> //strcmp

#include "WLGenerator.h"

using namespace std;

bool write_workload(const vector<vector<double> > &machine_loads, const vector<int> machines, const char *base_path);

void usage(const char* prog) {
    cout << "generates a workload for input into batchsim" << endl <<
        "Usage: " << prog << " <generator> <params>" << endl;
}

int main(int argc, char *argv[]) {
    int argi = 1;
    map<const char*,double> params;
    char *endptr;
    const char* workload_path = NULL;
    WLGenerator *generator = NULL;
    while(argi < argc) {
        if (argv[argi][0] == '=') {
            double t = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid value or none given";
                usage(argv[0]);
                return -1;
            }
            params[argv[argi-1]] = t;
            ++argi;
        } else if (!strcmp(argv[argi],"--")) {
            argi++;
            break;
        } else if (!strcmp(argv[argi],"--path")) {
            workload_path=argv[++argi];
            argi++;
        } else if (!strcmp(argv[argi],"--flat")) {
            if (generator != NULL) {
                delete generator;
            }
            generator = new FlatGenerator();
            argi++;
        } else if (!strcmp(argv[argi],"--lognormal")) {
            if (generator != NULL) {
                delete generator;
            }
            generator = new LogNormalGenerator();
            argi++;
        } else if (!strcmp(argv[argi],"--normal")) {
            if (generator != NULL) {
                delete generator;
            }
            generator = new NormalGenerator();
            argi++;
        } else {
            cout << "Unkown argument: " << argv[argi] << endl;
            usage(argv[0]);
            return -1;
        }
    }

    if (workload_path == NULL) {
        cout << "must specify workload base path" << endl;
        usage(argv[0]);
        return -1;
    }

    generator->init(params);
    vector<vector<double> > machine_loads;
    vector<int> machines;
    generator->generate(machine_loads, machines);
    write_workload(machine_loads, machines, workload_path);
    return 0;
}

bool write_workload(const vector<vector<double> > &machine_loads, const vector<int> machines, const char *base_path) {
    stringstream machinelist;
    machinelist << base_path << "_list";
    ofstream ml;
    ofstream of;
    ml.open(machinelist.str().c_str());
    if (!ml.is_open()) {
        cout << "Could not open workload file: " << machinelist.str() << endl;
        return false;
    }
    int machine_index = 0;
    for(vector<vector<double> >::const_iterator machine = machine_loads.begin(); machine != machine_loads.end(); ++machine) {
        stringstream ss;
        ss << base_path << "_" << machine_index;
        of.open(ss.str().c_str());
        if (!of.is_open()) {
            cout << "Could not open machine file: " << ss.str() << endl;
            ml.close();
            return false;
        }

        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            of << (*vm) << endl;
        }
        of.close();
        machine_index++;
    }

    for(vector<int>::const_iterator machine = machines.begin(); machine != machines.end(); ++machine) {
        stringstream ss;
        ss << base_path << "_" << *machine;
        ml << ss.str() << endl;
    }
    ml.close();
}
