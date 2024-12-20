#include <vector>
#include <string>

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
const vector<float> a = {0, .0001, .0002, .0003, .0004, .0005, .0006, .0007, .0008, .0009, .001};
const vector<float> b = {0, .1, .2, .3, .4, .5, .6, .7, .8, .9, 1, .9, .8, .7, .6, .5, .4, .3, .2, .1};
const float F_UP = 0.9;
const float F_DOWN = 0.001;
const float DELTA_F = F_UP - F_DOWN;

const string BAND_PASS_FILTER = "band_pass_filter";
const string NOTCH_FILTER = "notch_filter";
const string FIR_FILTER = "FIR_filter";
const string IIR_FILTER = "IIR_filter";
const string ERROR_JOIN = "Error joining thread ";
const string ERROR_CREATE = "Error creating thread ";
const string MILLISECONDS = " milliseconds.\n";
const string THREAD_NUMBER = ": Thread_number: ";
const string APPLY_FILTER_COMPLETED = " applied in ";
const string OUTPUT = "output_";
const string PAR_FORMAT = "_parallel.wav";
const string PARALLEL = "Parallel ";
const string INPUT_FILES_ERROR = "Please provide an input file name";
const string TOTAL_EXECUTION_TIME = "Total execution time: ";
const string READ_TIME = "Read Time: ";