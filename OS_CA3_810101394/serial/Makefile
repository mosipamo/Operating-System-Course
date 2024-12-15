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
	rm -f $(EXEC) output_Band_Pass_Filter_serial.wav output_Notch_Filter_serial.wav output_FIR_Filter_serial.wav output_IIR_Filter_serial.wav