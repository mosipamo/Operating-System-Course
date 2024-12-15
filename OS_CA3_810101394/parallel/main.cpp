#include <bits/stdc++.h>
#include <sndfile.h>
#include <pthread.h>
#include "const.hpp"

using namespace std;

vector<float> audioData;
vector<float> band_pass_result;
vector<float> notch_result;
vector<float> FIR_result;
vector<float> IIR_result;

float total_band_pass = 0;
float total_notch = 0;
float total_FIR = 0;
float total_IIR = 0;
float total_read_time = 0;
int num_threads;
int total_execution_time = 0;

struct filter_thread_args {
    int start;
    int end;
    int thread_number;
    string filter_name;
};

void read_wav_file(const string& inputFile, vector<float>& data, SF_INFO& fileInfo) {
    auto start = chrono::high_resolution_clock::now();
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
    auto end = chrono::high_resolution_clock::now();
    auto total_read_time = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Successfully read " << numFrames << " frames from " << inputFile << endl;
    cout << READ_TIME << total_read_time.count() << endl;
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

// Band_Pass_Filter
void* band_pass_algorithm(void* args) {
    filter_thread_args* thread_args = (filter_thread_args*)args;
    for (int i = thread_args->start; i < thread_args->end; i++) {
        if (audioData[i] >= F_DOWN and audioData[i] <= F_UP) {
            band_pass_result[i] = (audioData[i] * audioData[i]) / 
                            (audioData[i] * audioData[i] + DELTA_F * DELTA_F);
        }
        else band_pass_result[i] = 0.0f;
    }
    // cout << thread_args->filter_name << THREAD_NUMBER << thread_args->thread_number << endl;
    return nullptr;
}

// Notch_Filter
void* notch_algorithm(void* args) {
    filter_thread_args* thread_args = (filter_thread_args*)args;
    for (int i = thread_args->start; i < thread_args->end; i++) {
        notch_result[i] = 1.0f / (powf(audioData[i] / F0, 2 * n) + 1);
    }
    // cout << thread_args->filter_name << THREAD_NUMBER << thread_args->thread_number << endl;
    return nullptr;
}

// FIR_filter
void* FIR_algorithm(void* args) {
    filter_thread_args* thread_args = (filter_thread_args*)args;
    for (int n = thread_args->start; n < thread_args->end; n++) {
        for (int k = 0; k < h.size(); k++) {
            if (n - k >= 0)
                FIR_result[n] += h[k] * audioData[n - k];
        }
    }
    // cout << thread_args->filter_name << THREAD_NUMBER << thread_args->thread_number << endl;
    return nullptr;
}

// IIR_filter
void* IIR_algorithm(void* args) {
    filter_thread_args* thread_args = (filter_thread_args*)args;
    for (int n = thread_args->start; n < thread_args->end; n++) {
        for (int i = 0; i < b.size(); i++) {
            if (n - i >= 0)
                IIR_result[n] += audioData[n - i] * b[i];
        }
    }
    // cout << thread_args->filter_name << THREAD_NUMBER << thread_args->thread_number << endl;
    return nullptr;
}

// apply_filter function
void apply_filter(const vector<float>& inputData, SF_INFO fileInfo, void* (*filterAlgorithm)(void*), vector<float>& result, const string& filter_name) {
    audioData = inputData;
    result.resize(audioData.size());
    num_threads = thread::hardware_concurrency();
    if (num_threads == 0) 
        num_threads = 4;

    vector<pthread_t> threads(num_threads);
    vector<filter_thread_args> thread_args(num_threads);
    int chunkSize = audioData.size() / num_threads;

    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; i++) {
        thread_args[i].start = i * chunkSize;
        thread_args[i].end = (i == num_threads - 1) ? audioData.size() : (i + 1) * chunkSize;
        thread_args[i].thread_number = i + 1;
        thread_args[i].filter_name = filter_name;
        if (pthread_create(&threads[i], nullptr, filterAlgorithm, &thread_args[i]) != 0) {
            cerr << ERROR_CREATE << i << endl;
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], nullptr) != 0) {
            cerr << ERROR_JOIN << i << endl;
            exit(1);
        }
    }

    // serial part of IIR_filter
    if (filter_name == IIR_FILTER) {
        for (int n = 0; n < audioData.size(); n++) {
            for (int i = 0; i < a.size(); i++) {
                if (n - i >= 0)
                    IIR_result[n] -= IIR_result[n - i] * a[i];
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

    string output_name = OUTPUT + filter_name + PAR_FORMAT;
    string msg = PARALLEL + filter_name;
    write_wav_file(output_name, result, fileInfo);
    cout << msg << APPLY_FILTER_COMPLETED << elapsed.count() << MILLISECONDS;
    total_execution_time += elapsed.count();
}

void test_res() {
    for (int i = 0; i < band_pass_result.size(); i++)
        total_band_pass += band_pass_result[i];

    for (int i = 0; i < notch_result.size(); i++)
        total_notch += notch_result[i];

    for (int i = 0; i < FIR_result.size(); i++)
        total_FIR += FIR_result[i];

    for (int i = 0; i < IIR_result.size(); i++)
        total_IIR += IIR_result[i];

    cout << "test: " << total_band_pass << " " << total_notch << " " << total_FIR << " " << total_IIR << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << INPUT_FILES_ERROR << endl;
        return 0;
    }
    string inputFile = argv[1];
    SF_INFO fileInfo;
    memset(&fileInfo, 0, sizeof(fileInfo));

    read_wav_file(inputFile, audioData, fileInfo);

    // apply filter
    apply_filter(audioData, fileInfo, band_pass_algorithm, band_pass_result, BAND_PASS_FILTER);
    apply_filter(audioData, fileInfo, notch_algorithm, notch_result, NOTCH_FILTER);
    apply_filter(audioData, fileInfo, FIR_algorithm, FIR_result, FIR_FILTER);
    apply_filter(audioData, fileInfo, IIR_algorithm, IIR_result, IIR_FILTER);


    cout << TOTAL_EXECUTION_TIME << total_execution_time << endl;
    // test_res();
    
    return 0;
}