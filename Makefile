CXX ?= c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I.
SOURCES = setup.cpp hash.cpp block_h.cpp blockchain.cpp mining.cpp
HEADERS = $(wildcard *.hpp)

all: blockchain

blockchain: main.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp $(SOURCES) -o blockchain

run_tests: tests/tests.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/tests.cpp $(SOURCES) -o run_tests

test: run_tests
	./run_tests

clean:
	rm -f blockchain run_tests

.PHONY: all test clean
