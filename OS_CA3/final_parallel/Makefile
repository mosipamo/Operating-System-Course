CXX = g++
CXXFLAGS = -std=c++11
LDFLAGS = -lsndfile

SRC = main.cpp
HEADERS = const.hpp
EXEC = VoiceFilters.out

all: $(EXEC)

$(EXEC): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(EXEC) output_band_pass_filter_parallel.wav output_notch_filter_parallel.wav output_FIR_filter_parallel.wav output_IIR_filter_parallel.wav