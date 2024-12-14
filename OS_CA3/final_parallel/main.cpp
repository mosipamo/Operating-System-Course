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

int num_threads;
int total_time = 0;

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

// Band_Pass_Filter
void* band_pass_algorithm(void* args) {
    filter_thread_args* threadArgs = (filter_thread_args*)args;

    for (int i = threadArgs->start; i < threadArgs->end; i++) {
        if (audioData[i] >= F_DOWN and audioData[i] <= F_UP) {
            band_pass_result[i] = (audioData[i] * audioData[i]) / 
                            (audioData[i] * audioData[i] + DELTA_F * DELTA_F);
        }
        else band_pass_result[i] = 0.0f;
    }
    return nullptr;
}

// Notch_Filter
void* notch_algorithm(void* args) {
    filter_thread_args* threadArgs = (filter_thread_args*)args;
    for (int i = threadArgs->start; i < threadArgs->end; i++) {
        notch_result[i] = 1.0f / (powf(audioData[i] / F0, 2 * n) + 1);
    }
    return nullptr;
}

// FIR_filter
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

// IIR_filter
void* IIR_algorithm(void* args) {
    filter_thread_args* threadArgs = (filter_thread_args*)args;
    for (int n = threadArgs->start; n < threadArgs->end; n++) {
        for (int i = 0; i < b.size(); i++) {
            if (n - i >= 0)
                IIR_result[n] += audioData[n - i] * b[i];
        }
    }
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
    vector<filter_thread_args> threadArgs(num_threads);
    int chunkSize = audioData.size() / num_threads;
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; i++) {
        threadArgs[i].start = i * chunkSize;
        threadArgs[i].end = (i == num_threads - 1) ? audioData.size() : (i + 1) * chunkSize;
        if (pthread_create(&threads[i], nullptr, filterAlgorithm, &threadArgs[i]) != 0) {
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

    if (filter_name == "IIR_filter") {
        // serial part of IIR_filter
        for (int n = 0; n < audioData.size(); n++) {
            for (int i = 0; i < a.size(); i++) {
                if (n - i >= 0)
                    IIR_result[n] -= IIR_result[n - i] * a[i];
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

    string output_name = "output_" + filter_name + "_parallel.wav";
    string msg = "Parallel " + filter_name;
    write_wav_file(output_name, result, fileInfo);
    cout << msg << " applied in " << elapsed.count() << " milliseconds.\n";
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

    // apply filter
    apply_filter(audioData, fileInfo, band_pass_algorithm, band_pass_result, "band_pass_filter");
    apply_filter(audioData, fileInfo, notch_algorithm, notch_result, "notch_filter");
    apply_filter(audioData, fileInfo, FIR_algorithm, FIR_result, "FIR_filter");
    apply_filter(audioData, fileInfo, IIR_algorithm, IIR_result, "IIR_filter");

    cout << "Total time: " << total_time << endl;

    return 0;
}