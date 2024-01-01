#include <string>
#include <vector>

using namespace std;

class Firewall
{
private:
    enum { Connect = 1, Bind = 2 };
    vector<string> connect_rules;
    vector<string> bind_rules;
public:
    Firewall();
    bool check(int status, string dstip);
    ~Firewall();
};


