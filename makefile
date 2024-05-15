# Compiler
CXX = g++
# Compiler flags
CXXFLAGS := -std=c++14 -O2

# Include directories
INCLUDEDIRS := .
# Include files
INCLUDES := $(wildcard *.hpp)

# Source directories
SOURCEDIRS := .
# Source files
SOURCES := $(wildcard $(patsubst %, %/*.cpp, $(SOURCEDIRS)))

# Target files
TARGETS := $(patsubst %.cpp, %, $(SOURCES))


all: $(TARGETS)

$(TARGETS): %: %.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) -I$(INCLUDEDIRS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(TARGETS)
