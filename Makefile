SRCS  =    \
	parser.cpp \
	lru_stl.cpp \
	global.cpp \
	stats.cpp\
	main.cpp \
	



OBJS :=   $(SRCS:%.cpp=objs/%.o)

ERR = $(shell icpc 2>/dev/null ; echo $? )

ifeq "$(ERR)" "127"
    CXX = g++
else
    CXX = icpc
endif

CPPFLAG = -g -DDEBUG -DVERB -std=c++0x -Wall

all: main

main: $(OBJS)
	$(CXX) -o sim-ideal  $(OBJS) $(LDFLAG) $(CPPFLAG)


objs/%.o: %.cpp
	$(CXX) $(CPPFLAG) -c $< -o $@

clean:
	rm -rf objs/*.o
	rm -rf sim-ideal
