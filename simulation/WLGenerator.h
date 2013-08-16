#include <map>
#include <string>
#include <vector>

using namespace std;

class WLGenerator {
    public :
        virtual void init(map<string,double> params) = 0;
        virtual void generate(vector<vector<double> > &workload) = 0;
};
