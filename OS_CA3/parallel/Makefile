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
	rm -f $(EXEC) output_band_pass_filter.wav output_notch_filter.wav output_FIR_filter.wav output_IIR_filter.wav