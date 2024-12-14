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
	rm -f $(EXEC) output_Band_Pass_Filter.wav output_Notch_Filter.wav output_FIR_Filter.wav output_IIR_Filter.wav