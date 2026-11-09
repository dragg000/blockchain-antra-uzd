CXX ?= c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I. -pthread
SOURCES = setup.cpp hash.cpp merkle.cpp block_h.cpp blockchain.cpp mining.cpp validation.cpp
HEADERS = $(wildcard *.hpp)

all: blockchain

blockchain: main.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp $(SOURCES) -o blockchain

run_tests: tests/tests.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/tests.cpp $(SOURCES) -o run_tests

run_tests_v03: tests/tests_v03.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/tests_v03.cpp $(SOURCES) -o run_tests_v03

# run_tests yra bendras v0.2 ir v0.3 rinkinys, run_tests_v03 tikrina tik v0.3 pakeitimus.
test: run_tests run_tests_v03
	./run_tests
	./run_tests_v03

run_bench: bench.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) bench.cpp $(SOURCES) -o run_bench

run_bench_parallel: bench_parallel.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) bench_parallel.cpp $(SOURCES) -o run_bench_parallel

# run_bench yra tas pats matavimas kaip v0.2, run_bench_parallel lygina nuosekliu ir lygiagretu kasima.
bench: run_bench run_bench_parallel
	./run_bench
	./run_bench_parallel

clean:
	rm -f blockchain run_tests run_tests_v03 run_bench run_bench_parallel

.PHONY: all test bench clean
