SRCS  =    \
	parser.cpp \
	lru_stl.cpp \
	global.cpp \
	stats.cpp\
	min.cpp\
	main.cpp \
	



OBJS :=   $(SRCS:%.cpp=objs/%.o)

ERR = $(shell which icpc >/dev/null; echo $?)
ifeq "$(ERR)" "0"
    CXX = icpc
else
    CXX = g++
endif

CPPFLAG = -g -DDEBUG -DVERB -std=c++0x -Wall
CPPFLAG = -g -DDEBUG  -std=c++0x -Wall

all: main

main: $(OBJS)
	$(CXX) -o sim-ideal  $(OBJS) $(LDFLAG) $(CPPFLAG)


objs/%.o: %.cpp
	$(CXX) $(CPPFLAG) -c $< -o $@

clean:
	rm -rf objs/*.o
	rm -rf sim-ideal
