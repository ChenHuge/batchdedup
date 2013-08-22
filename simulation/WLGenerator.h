#include <map>
#include <string>
#include <vector>

using namespace std;

class WLGenerator {
    public :
        virtual void init(map<const char*,double> params) = 0;
        virtual void generate(vector<vector<double> > &workload, vector<int> &machines) = 0;
};

class FlatGenerator : public WLGenerator {
    public :
        void init(map<const char*,double> params);
        void generate(vector<vector<double> > &workload, vector<int> &machines);
        FlatGenerator();
    private:
        int VMs;
        int p;
        double size;
};

class NormalGenerator : public WLGenerator {
    public :
        void init(map<const char*,double> params);
        void generate(vector<vector<double> > &workload, vector<int> &machines);
        NormalGenerator();
    private:
        int VMs;
        int p;
        double MaxSize;
        double MinSize;
        double AvgSize;
        double Variance;
};

class LogNormalGenerator : public WLGenerator {
    public :
        void init(map<const char*,double> params);
        void generate(vector<vector<double> > &workload, vector<int> &machines);
        LogNormalGenerator();
    private:
        int VMs;
        int p;
        double MaxSize;
        double MinSize;
        double AvgSize;
        double Variance;
};
