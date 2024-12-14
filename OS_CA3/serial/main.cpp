#include <bits/stdc++.h>
#include <sndfile.h>
#include "const.hpp"

using namespace std;

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
vector<float> apply_Band_Pass_Filter(vector<float> data) {
    vector<float> ans;
    for (int i = 0; i < data.size(); i++) {
        if (data[i] >= F_DOWN && data[i] <= F_UP)
            ans.pb((data[i] * data[i]) / (data[i] * data[i] + DELTA_F * DELTA_F));
        else 
            ans.pb(0.0f);
    }
    return ans;
}

void Band_Pass_filter_func(const vector<float>& audioData, SF_INFO fifoInfo_band) {
    string outputFile = "output_Band_Pass_Filter.wav";
    auto start = chrono::high_resolution_clock::now();
    vector<float> Band_pass_filter_audio = apply_Band_Pass_Filter(audioData);
    auto end = chrono::high_resolution_clock::now();
    auto Band_pass_filter_time = chrono::duration_cast<chrono::milliseconds>(end - start);
    write_wav_file(outputFile, Band_pass_filter_audio, fifoInfo_band);
    cout << "Band-pass-filter applied in " << Band_pass_filter_time.count() << " milliseconds.\n";
    total_time += Band_pass_filter_time.count();
}

// Notch_Filter
vector<float> apply_Notch_Filter(vector<float> data) {
    vector<float> ans;
    for (int i = 0; i < data.size(); i++)
        ans.pb(1 / (powf(data[i] / F0, 2*n) + 1));
    return ans;
}

void Notch_filter_func(const vector<float>& audioData, SF_INFO fifoInfo_band) {
    string outputFile = "output_Notch_Filter.wav";
    auto start = chrono::high_resolution_clock::now();
    vector<float> Notch_filter_audio = apply_Notch_Filter(audioData);
    auto end = chrono::high_resolution_clock::now();
    auto Notch_filter_time = chrono::duration_cast<chrono::milliseconds>(end - start);
    write_wav_file(outputFile, Notch_filter_audio, fifoInfo_band);
    cout << "Notch-filter applied in " << Notch_filter_time.count() << " milliseconds.\n";
    total_time += Notch_filter_time.count();
}


// FIR_filter
vector<float> apply_FIR_Filter(const vector<float>& x) {
    vector<float> y(x.size(), 0.0f);

    for (int n = 0; n < x.size(); n++)
        for (int k = 0; k < h.size(); k++)
            if (n - k >= 0)
                y[n] += h[k] * x[n - k];
    return y;
}

void FIR_filter_func(const vector<float>& audioData, SF_INFO fifoInfo_band) {
    string outputFile = "output_FIR_Filter.wav";
    auto start = chrono::high_resolution_clock::now();
    vector<float> FIR_audio = apply_FIR_Filter(audioData);
    auto end = chrono::high_resolution_clock::now();
    auto FIR_filter_time = chrono::duration_cast<chrono::milliseconds>(end - start);
    write_wav_file(outputFile, FIR_audio, fifoInfo_band);
    cout << "FIR-filter applied in " << FIR_filter_time.count() << " milliseconds.\n";
    total_time += FIR_filter_time.count();
}

// IIR_filter
vector<float> apply_IIR_Filter(const vector<float>& x) {
    vector<float> y(x.size(), 0.0f);
    for (int n = 0; n < x.size(); n++) {
        for (int i = 0; i < b.size(); i++) {
            if (n - i >= 0)
                y[n] += x[n - i] * b[i];
        }
        for (int i = 0; i < a.size(); i++) {
            if (n - i >= 0)
                y[n] -= y[n - i] * a[i];
        }
    }
    return y;
}

void IIR_filter_func(const vector<float>& audioData, SF_INFO fifoInfo_band) {
    string outputFile = "output_IIR_Filter.wav";
    auto start = chrono::high_resolution_clock::now();
    vector<float> IIR_audio = apply_IIR_Filter(audioData);
    auto end = chrono::high_resolution_clock::now();
    auto IIR_filter_time = chrono::duration_cast<chrono::milliseconds>(end - start);
    write_wav_file(outputFile, IIR_audio, fifoInfo_band);
    cout << "IIR-filter applied in " << IIR_filter_time.count() << " milliseconds.\n";
    total_time += IIR_filter_time.count();
}

int main(int argc, char* argv[]) {
    if (argc < 2)
    {
        cout << "Please provide an input file name" << endl;
        return 0;
    }
    cout << ((float) rand()/RAND_MAX) << endl;

    string inputFile = argv[1];
    string outputFileMain = "output.wav";
    
    SF_INFO fileInfo;
    vector<float> audioData;
    memset(&fileInfo, 0, sizeof(fileInfo));

    read_wav_file(inputFile, audioData, fileInfo);
    SF_INFO fifoInfo_pass_filter_band = fileInfo;
    SF_INFO fifoInfo_notch_filter = fileInfo;
    SF_INFO fifoInfo_FIR_filter = fileInfo;
    SF_INFO fifoInfo_IIR_filter = fileInfo;

    // apply filter
    Band_Pass_filter_func(audioData, fifoInfo_pass_filter_band);
    Notch_filter_func(audioData, fifoInfo_notch_filter);
    FIR_filter_func(audioData, fifoInfo_FIR_filter);
    IIR_filter_func(audioData, fifoInfo_IIR_filter);

    cout << "Total time: " << total_time << endl;

    return 0;
}