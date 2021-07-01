CXX = g++

CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17
ifeq (${optimize}, no)
CXXFLAGS := ${CXXFLAGS} -g -DDEBUG_PRINT
else
CXXFLAGS := ${CXXFLAGS} -O4
endif
target = ldrawing
objects = main.o rectangular_dual.o port_assignment.o decompose.o timer.o

.PHONY: all
all: ${target} sample-triangulation test-planar

${target}: ${objects}
	${CXX} $^ -o $@

.PHONY: clean
clean:
	rm -f ${objects} ${target} sampling.o sample-triangulation test-planar
