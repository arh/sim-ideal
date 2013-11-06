CXX=g++
CXXFLAGS=-Wall -c -g -O0
LDFLAGS=
HEADERS=ssd.h
SOURCES=ssd_address.cpp ssd_block.cpp ssd_bm.cpp ssd_bus.cpp \
		ssd_channel.cpp ssd_controller.cpp ssd_die.cpp \
		ssd_event.cpp ssd_config.cpp ssd_ftlparent.cpp ssd_gc.cpp \
		ssd_package.cpp ssd_page.cpp ssd_plane.cpp ssd_ram.cpp \
		ssd_stats.cpp ssd_ssd.cpp ssd_wl.cpp \
		dftl_parent.cpp dftl_ftl.cpp bdftl_ftl.cpp \
		fast_ftl.cpp page_ftl.cpp bast_ftl.cpp \
		run_ufliptrace.cpp
		#run_test.cpp
OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) flashsim

flashsim: $(HEADERS) $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o: $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	-rm -rf *.o flashsim
