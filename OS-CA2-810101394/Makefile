# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall

# Source files and headers
SOURCES_MAIN = main.cpp logger.cpp
SOURCES_WAREHOUSE = warehouse.cpp logger.cpp
SOURCES_ITEM = item.cpp logger.cpp
HEADERS = colorprint.hpp const.hpp logger.hpp

# Executables
EXECUTABLE_MAIN = main
EXECUTABLE_WAREHOUSE = warehouse
EXECUTABLE_ITEM = item

# Default target
all: $(EXECUTABLE_MAIN) $(EXECUTABLE_WAREHOUSE) $(EXECUTABLE_ITEM)

# Rules for building main executable
$(EXECUTABLE_MAIN): $(SOURCES_MAIN) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES_MAIN)

# Rules for building warehouse executable
$(EXECUTABLE_WAREHOUSE): $(SOURCES_WAREHOUSE) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES_WAREHOUSE)

# Rules for building item executable
$(EXECUTABLE_ITEM): $(SOURCES_ITEM) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES_ITEM)

# Clean up build artifacts
clean:
	rm -f $(EXECUTABLE_MAIN) $(EXECUTABLE_WAREHOUSE) $(EXECUTABLE_ITEM)

# Phony targets
.PHONY: all clean
