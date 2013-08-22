#include "WLGenerator.h"
#include <cstdlib> //rand,srand
#include <ctime> //time (for seed)
#include <cmath>
#include <iostream>
#include <cstring>

using namespace std;

void rand_init() {
    srand(time(NULL));
}

//returns uniform random number in closed interval [0,1]
double rand_float() {
    return (double)rand() / RAND_MAX;
}

//polar form of Box-Muller algorithm (Marsaglia polar method)
double rand_normal(double mean, double stddev) {
    float u, v, w, x, y;
    do {
        u = 2.0 * rand_float() - 1.0;
        v = 2.0 * rand_float() - 1.0;
        w = u * u + v * v;
    } while ( w >= 1.0 );

    w = sqrt( (-2.0 * log( w ) ) / w );
    x = u * w;
    //y = v * w; //we just throw away y, even though it is an extra result
    return mean+x*stddev;
}

void lognormal_params(double mean, double variance, double &mu, double &sigma) {
    mu = log(mean*mean/sqrt(variance+mean*mean));
    sigma = sqrt(log(1+variance/(mean*mean)));
}

double rand_lognormal(double mu, double sigma) {
    return exp(mu + rand_normal(0.0,1.0)*sigma);
}

double rand_skewnormal(double mean, double stddev, double skew) {
    return 0; //I don't quite have lognormal figured out, but maybe it is best
}

FlatGenerator::FlatGenerator() {
    VMs = 2500;
    p = 100;
    size=40.0;
}

void FlatGenerator::init(map<const char*,double> params) {
    map<const char*,double>::iterator it;
    for(it = params.begin(); it != params.end(); ++it) {
        //size, VMs, p
        if (!strcmp(it->first,"=size")) {
            size = it->second;
        } else if (!strcmp(it->first,"=vms")) {
            VMs = it->second;
        } else if (!strcmp(it->first,"=v")) {
            VMs = it->second * p;
        } else if (!strcmp(it->first,"=p")) {
            p = it->second;
        } else {
            cout << "Unknown argument: " << it->first << endl;
        } 
    }
}

void FlatGenerator::generate(vector<vector<double> > &workload, vector<int> &machines) {
    vector<double> machine;
    for(int i = 0; i < VMs/p; i++) {
        machine.push_back(size);
    }
    workload.push_back(machine);
    for(int i = 0; i < p; i++) {
        machines.push_back(0);
    }
}

NormalGenerator::NormalGenerator() {
    VMs = 2500;
    p = 100;
    MaxSize = 4096.0;
    MinSize = 1.0;
    AvgSize = 40.0;
    Variance = 20.0;
    rand_init();
}

void NormalGenerator::init(map<const char*,double> params) {
    map<const char*,double>::iterator it;

    for(it = params.begin(); it != params.end(); ++it) {
        //MaxSize, MinSize, AvgSize, Variance, VMs, p
        if (!strcmp(it->first,"=avg")) {
            AvgSize = it->second;
        } else if (!strcmp(it->first,"=min")) {
            MinSize = it->second;
        } else if (!strcmp(it->first,"=max")) {
            MaxSize = it->second;
        } else if (!strcmp(it->first,"=var")) {
            Variance = it->second;
        } else if (!strcmp(it->first,"=vms")) {
            VMs = it->second;
        } else if (!strcmp(it->first,"=v")) {
            VMs = it->second * p;
        } else if (!strcmp(it->first,"=p")) {
            p = it->second;
        } else {
            cout << "Unknown argument: " << it->first << endl;
        } 
    }
}

void NormalGenerator::generate(vector<vector<double> > &workload, vector<int> &machines) {
    for(int i = 0; i < p; i++) {
        vector<double> machine;
        for(int i = 0; i < VMs/p; i++) {
            double size = rand_normal(AvgSize, Variance);
            while (size < MinSize || size > MaxSize) {
                size = rand_normal(AvgSize, Variance);
            }
            cout << size << endl;
            machine.push_back(size);
        }
        workload.push_back(machine);
    }
    for(int i = 0; i < p; i++) {
        machines.push_back(i);
    }
}

LogNormalGenerator::LogNormalGenerator() {
    VMs = 2500;
    p = 100;
    MaxSize = 4096.0;
    MinSize = 1.0;
    AvgSize = 40.0;
    Variance = 1024.0;
    rand_init();
}

void LogNormalGenerator::init(map<const char*,double> params) {
    map<const char*,double>::iterator it;

    for(it = params.begin(); it != params.end(); ++it) {
        //MaxSize, MinSize, AvgSize, Variance, VMs, p
        if (!strcmp(it->first,"=avg")) {
            AvgSize = it->second;
        } else if (!strcmp(it->first,"=min")) {
            MinSize = it->second;
        } else if (!strcmp(it->first,"=max")) {
            MaxSize = it->second;
        } else if (!strcmp(it->first,"=var")) {
            Variance = it->second;
        } else if (!strcmp(it->first,"=vms")) {
            VMs = it->second;
        } else if (!strcmp(it->first,"=v")) {
            VMs = it->second * p;
        } else if (!strcmp(it->first,"=p")) {
            p = it->second;
        } else {
            cout << "Unknown argument: " << it->first << endl;
        } 
    }
}

void LogNormalGenerator::generate(vector<vector<double> > &workload, vector<int> &machines) {
    double mu, sigma;
    lognormal_params(AvgSize, Variance, mu, sigma);
    for(int i = 0; i < p; i++) {
        vector<double> machine;
        for(int i = 0; i < VMs/p; i++) {
            double size = rand_lognormal(mu, sigma);
            while (size < MinSize || size > MaxSize) {
                size = rand_lognormal(mu, sigma);
            }
            cout << size << endl;
            machine.push_back(size);
        }
        workload.push_back(machine);
    }
    for(int i = 0; i < p; i++) {
        machines.push_back(i);
    }
}
