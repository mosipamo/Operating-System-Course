#include <vector>
using namespace std;

#define COMMA ','
#define SPACE " "
#define TAB "\t"
#define ll long long int
#define pb push_back
#define mp make_pair
#define ld long double
#define F first
#define S second
#define endl "\n"

const float F0 = 1.0f;
const int n = 1;
const vector<float> h = {.1, .1, .1, .1, .1, .1, .1, .1, .1, .1};
vector<float> b = {0, .1, .2, .3, .4, .5, .6, .7, .8, .9, 1, .9, .8, .7, .6, .5, .4, .3, .2, .1};
vector<float> a = {0, .0001, .0002, .0003, .0004, .0005, .0006, .0007, .0008, .0009, .001};
int total_time = 0;
const float F_UP = 0.9;
const float F_DOWN = 0.001;
const float DELTA_F = F_UP - F_DOWN;