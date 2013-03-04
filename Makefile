SRCS=$(wildcard src/*.cpp)
OBJS :=   $(SRCS:%.cpp=%.o)

ERR = $(shell which icpc >/dev/null; echo $$?)
ifeq "$(ERR)" "0"
    CXX = icpc
else
    CXX = g++
endif

CPPFLAG = -g -DDEBUG -DVERB -std=c++0x -Wall
# CPPFLAG = -g -DDEBUG  -std=c++0x -Wall
# CPPFLAG = -g -DDEBUG  -std=c++0x -Wall -DREQSIZE
#CPPFLAG = -O3 -DNDEBUG -std=c++0x -Wall -DREQSIZE
# CPPFLAG = -O3 -DNDEBUG -std=c++0x -Wall
#CPPFLAG = -O3 -DNDEBUG -std=c++0x -Wall -DHIST -DREQSIZE


all: main

main: $(OBJS)
	$(CXX) -o sim-ideal  $(OBJS) $(LDFLAG) $(CPPFLAG)


%.o: %.cpp
	$(CXX) $(CPPFLAG) -c $< -o $@

clean:
	rm -f src/*.o
	rm -f sim-ideal
