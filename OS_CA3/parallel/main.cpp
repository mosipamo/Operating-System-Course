#include <bits/stdc++.h>
#include <sndfile.h>
#include <pthread.h>

#define ll long long int
#define pb push_back
#define mp make_pair
#define ld long double
#define F first
#define S second
#define debug() cout << "fuck\n"

using namespace std;

vector<float> audioData;
vector<float> band_pass_result;
vector<float> notch_result;
vector<float> FIR_result;
vector<float> IIR_result;

int num_threads;
int total_time = 0;

const float DELTA_F = 1.0f;
const float F0 = 1.0f;
const int n = 1;
const vector<float> h = {.1, .1, .1, .1, .1, .1, .1, .1, .1, .1};
const vector<float> a = {0, .0001, .0002, .0003, .0004, .0005, .0006, .0007, .0008, .0009, .001};
const vector<float> b = {0, .1, .2, .3, .4, .5, .6, .7, .8, .9, 1, .9, .8, .7, .6, .5, .4, .3, .2, .1};

struct filter_thread_args {
    int start;
    int end;
};

void read_wav_file(const string& inputFile, vector<float>& data, SF_INFO& fileInfo) {
    SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
    if (!inFile) {
        cerr << "Error opening input file: " << sf_strerror(NULL) << endl;
        exit(1);
    }

    data.resize(fileInfo.frames * fileInfo.channels);
    sf_count_t numFrames = sf_readf_float(inFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        cerr << "Error reading frames from file." << endl;
        sf_close(inFile);
        exit(1);
    }

    sf_close(inFile);
    cout << "Successfully read " << numFrames << " frames from " << inputFile << endl;
}

void write_wav_file(const string& outputFile, const vector<float>& data, SF_INFO& fileInfo) {
    sf_count_t originalFrames = fileInfo.frames;
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    if (!outFile) {
        cerr << "Error opening output file: " << sf_strerror(NULL) << endl;
        exit(1);
    }

    sf_count_t numFrames = sf_writef_float(outFile, data.data(), originalFrames);
    if (numFrames != originalFrames) {
        cerr << "Error writing frames to file." << endl;
        sf_close(outFile);
        exit(1);
    }

    sf_close(outFile);
    cout << "Successfully wrote " << numFrames << " frames to " << outputFile << endl;
}

void* band_pass_algorithm(void* args) {
    filter_thread_args* threadArgs = (filter_thread_args*)args;

    for (int i = threadArgs->start; i < threadArgs->end; i++) {
        band_pass_result[i] = (audioData[i] * audioData[i]) / 
                            (audioData[i] * audioData[i] + DELTA_F * DELTA_F);
    }
    return nullptr;
}

void band_pass_filter_parallel(const vector<float>& inputData, SF_INFO fileInfo) {
    audioData = inputData;
    band_pass_result.resize(audioData.size());
    num_threads = thread::hardware_concurrency();
    if (num_threads == 0) 
        num_threads = 4; 

    vector<pthread_t> threads(num_threads);
    vector<filter_thread_args> threadArgs(num_threads);
    int chunkSize = audioData.size() / num_threads;
    cout << chunkSize << endl;
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; i++) {
        threadArgs[i].start = i * chunkSize;
        threadArgs[i].end = (i == num_threads - 1) ? audioData.size() : (i + 1) * chunkSize;
        if (pthread_create(&threads[i], nullptr, band_pass_algorithm, &threadArgs[i]) != 0) {
            cerr << "Error creating thread " << i << endl;
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], nullptr) != 0) {
            cerr << "Error joining thread " << i << endl;
            exit(1);
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

    write_wav_file("output_band_pass_filter_parallel.wav", band_pass_result, fileInfo);
    cout << "Parallel Band-pass filter applied in " << elapsed.count() << " milliseconds.\n";
    total_time += elapsed.count();
}

void* notch_algorithm(void* args) {
    filter_thread_args* threadArgs = (filter_thread_args*)args;
    for (int i = threadArgs->start; i < threadArgs->end; i++) {
        notch_result[i] = 1.0f / (powf(audioData[i] / F0, 2 * n) + 1);
    }
    return nullptr;
}

void notch_filter_parallel(const vector<float>& inputData, SF_INFO fileInfo) {
    audioData = inputData;
    notch_result.resize(audioData.size());
    num_threads = thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    vector<pthread_t> threads(num_threads);
    vector<filter_thread_args> threadArgs(num_threads);
    int chunkSize = audioData.size() / num_threads;

    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; i++) {
        threadArgs[i].start = i * chunkSize;
        threadArgs[i].end = (i == num_threads - 1) ? audioData.size() : (i + 1) * chunkSize;
        if (pthread_create(&threads[i], nullptr, notch_algorithm, &threadArgs[i]) != 0) {
            cerr << "Error creating thread " << i << endl;
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], nullptr) != 0) {
            cerr << "Error joining thread " << i << endl;
            exit(1);
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

    write_wav_file("output_notch_filter_parallel.wav", notch_result, fileInfo);
    cout << "Parallel Notch filter applied in " << elapsed.count() << " milliseconds.\n";
    total_time += elapsed.count();
}

void* FIR_algorithm(void* args) {
    filter_thread_args* threadArgs = (filter_thread_args*)args;
    for (int n = threadArgs->start; n < threadArgs->end; n++) {
        for (int k = 0; k < h.size(); k++) {
            if (n - k >= 0)
                FIR_result[n] += h[k] * audioData[n - k];
        }
    }
    return nullptr;
}

void FIR_filter_parallel(const vector<float>& inputData, SF_INFO fileInfo) {
    audioData = inputData;
    FIR_result.resize(audioData.size());
    num_threads = thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    vector<pthread_t> threads(num_threads);
    vector<filter_thread_args> threadArgs(num_threads);
    int chunkSize = audioData.size() / num_threads;

    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; i++) {
        threadArgs[i].start = i * chunkSize;
        threadArgs[i].end = (i == num_threads - 1) ? audioData.size() : (i + 1) * chunkSize;
        if (pthread_create(&threads[i], nullptr, FIR_algorithm, &threadArgs[i]) != 0) {
            cerr << "Error creating thread " << i << endl;
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], nullptr) != 0) {
            cerr << "Error joining thread " << i << endl;
            exit(1);
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);
    write_wav_file("output_FIR_filter_parallel.wav", FIR_result, fileInfo);
    cout << "Parallel FIR filter applied in " << elapsed.count() << " milliseconds.\n";
    total_time += elapsed.count();
}

void* IIR_algorithm(void* args) {
    filter_thread_args* threadArgs = (filter_thread_args*)args;
    for (int n = threadArgs->start; n < threadArgs->end; n++) {
        for (int i = 0; i < b.size(); i++) {
            if (n - i >= 0)
                IIR_result[n] += audioData[n - i] * b[i];
        }
        // for (int i = 0; i < a.size(); i++) {
        //     if (n - i >= 0)
        //         IIR_result[n] -= IIR_result[n - i] * a[i];
        // }
    }
    return nullptr;
}

void IIR_filter_parallel(const vector<float>& inputData, SF_INFO fileInfo) {
    audioData = inputData;
    IIR_result.resize(audioData.size());
    num_threads = thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;
    cout << "Num Threads: " << num_threads << endl;
    vector<pthread_t> threads(num_threads);
    vector<filter_thread_args> threadArgs(num_threads);
    int chunkSize = audioData.size() / num_threads;

    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; i++) {
        threadArgs[i].start = i * chunkSize;
        threadArgs[i].end = (i == num_threads - 1) ? audioData.size() : (i + 1) * chunkSize;
        if (pthread_create(&threads[i], nullptr, IIR_algorithm, &threadArgs[i]) != 0) {
            cerr << "Error creating thread " << i << endl;
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], nullptr) != 0) {
            cerr << "Error joining thread " << i << endl;
            exit(1);
        }
    }
    
    // serial part
    for (int n = 0; n < audioData.size(); n++) {
        for (int i = 0; i < a.size(); i++) {
            if (n - i >= 0)
                IIR_result[n] -= IIR_result[n - i] * a[i];
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);
    write_wav_file("output_IIR_filter_parallel.wav", IIR_result, fileInfo);
    cout << "Parallel IIR filter applied in " << elapsed.count() << " milliseconds.\n";
    total_time += elapsed.count();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Please provide an input file name" << endl;
        return 0;
    }

    string inputFile = argv[1];
    SF_INFO fileInfo;
    memset(&fileInfo, 0, sizeof(fileInfo));

    read_wav_file(inputFile, audioData, fileInfo);
    // SF_INFO fifoInfo_pass_filter_band = fileInfo;
    // SF_INFO fifoInfo_notch_filter = fileInfo;
    // SF_INFO fifoInfo_FIR_filter = fileInfo;
    // SF_INFO fifoInfo_IIR_filter = fileInfo;

    band_pass_filter_parallel(audioData, fileInfo);
    notch_filter_parallel(audioData, fileInfo);
    FIR_filter_parallel(audioData, fileInfo);
    IIR_filter_parallel(audioData, fileInfo);

    cout << "Total time: " << total_time << endl;

    return 0;
}
