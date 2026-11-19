CXX      := clang++
CXXFLAGS := -std=c++17 -pthread -O2 -Wall -Wextra
INCLUDES := -I. -Icore -Ihelpers -Iqueues -Iwalkers -Iworkers

MFIND_SRCS := mfind.cpp core/fileCache.cpp helpers/ignoreRules.cpp
TEST_SRCS  := tests/tests_main.cpp tests/test_ahoCorasick.cpp tests/test_matchIndex.cpp tests/test_search.cpp core/fileCache.cpp helpers/ignoreRules.cpp

.PHONY: all build test clean

all: build

build: mfind

HEADERS := $(wildcard core/*.h helpers/*.h queues/*.h walkers/*.h workers/*.h tests/*.h)

mfind: $(MFIND_SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(MFIND_SRCS) -o mfind

test: run_tests
	./run_tests

run_tests: $(TEST_SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRCS) -o run_tests

clean:
	rm -f mfind run_tests .match_index
	rm -rf .cache
