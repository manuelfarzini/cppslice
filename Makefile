# Project name
PROJ := cppslice
SHELL := /bin/sh

# Generic flags
CXX := g++
CXXFLAGS := -std=c++2b -Weffc++ -Wall -Wextra -Wshadow -Werror -pedantic -Iinclude
SUPPRESS := -Wno-pre-c++2b-compat -Wno-c++2b-extensions

# Compiler flags
DEBUG_FLAGS := -g -O0 -D_DEBUG
RELEASE_FLAGS := -O3 -DNDEBUG
TEST_FLAGS := -I/opt/homebrew/opt/googletest/include

# Linker flags
LDFLAGS :=
TEST_LDFLAGS := -L/opt/homebrew/Cellar/googletest/1.15.2/lib -lgtest -lgtest_main -pthread

# Set targets
TARGET := $(PROJ).x
TEST_TARGET := $(PROJ)_test.x

# Set files
CXX_SOURCES := $(shell find src -name "*.cpp")
TEST_SOURCES := $(shell find tests -name "*.cpp")
HEADERS := $(shell find include -name "*.h" -o -name "*.hpp")
OBJECTS := $(CXX_SOURCES:.cpp=.o)
TEST_OBJECTS := $(TEST_SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build target in debug mode
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(SUPPRESS) $(LDFLAGS) $(OBJECTS) -o $(TARGET)

# Build tests
test: $(filter-out src/main.o, $(OBJECTS)) $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(SUPPRESS) $(TEST_LDFLAGS) $(filter-out src/main.o, $(OBJECTS)) $(TEST_OBJECTS) -o $(TEST_TARGET)

# Release build
release: $(CXX_SOURCES)
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) $(SUPPRESS) $(LDFLAGS) $(CXX_SOURCES) -o $(TARGET)

# Create a zip archive of the project files
zip:
	-zip $(PROJ).zip "$(HEADERS)" "$(CXX_SOURCES)" Makefile GRADER_INFO.txt

# Clean build artifacts
clean:
	-rm -f $(TARGET) $(TEST_TARGET) $(OBJECTS) $(TEST_OBJECTS) $(PROJ).zip

# Pattern rule for compiling source files to object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $(DEBUG_FLAGS) $(SUPPRESS) -c -o $@ $<

# Phony targets
.PHONY: all release zip clean
