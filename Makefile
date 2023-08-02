# Variables
CXX = g++
CXXFLAGS = -std=c++23 -O3

# Targets
all: runnable

runnable: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f runnable

exec:
	./runnable
