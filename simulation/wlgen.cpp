#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "WLGenerator.h"

using namespace std;

bool write_workload(const vector<vector<double> > &machine_loads, const char *base_path);

int main(int argc, char *argv[]) {
    
}

bool write_workload(const vector<vector<double> > &machine_loads, const char *base_path) {
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
        ml << ss.str() << endl;

        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            of << (*vm) << endl;
        }
        of.close();
        machine_index++;
    }
    ml.close();
}
