

CXX = g++

CFLAGS = -g -O3 -I/usr/local/include/GraphicsMagick -I/usr/local/include/libusb-1.0
LIBS = 

ifeq ($(shell uname -s), Darwin)
	CFLAGS += -DMACOSX -L/usr/local/lib
	OGLFLAGS = -framework GLUT -framework OpenGL -framework Cocoa
else
	CFLAGS += -DLINUX
	OGLFLAGS = -lglut -lgl -lglu
endif


all: rigolserv rigolscope

obj:
	mkdir -p obj

rigolserv: obj src/simple_except.h src/freetmc.h src/netcomm.h obj/freetmc_local.cpp.o obj/rigolserv.cpp.o
	$(CXX) $(CFLAGS) obj/freetmc_local.cpp.o obj/rigolserv.cpp.o -L/usr/local/lib -lusb-1.0 -o rigolserv

rigolscope: obj src/simple_except.h src/freetmc.h src/netcomm.h src/remotedevice.h src/rigoltmc.h obj/rigolscope.cpp.o obj/plotting.cpp.o
	$(CXX) $(CFLAGS) ${OGLFLAGS} `GraphicsMagick++-config --cxxflags --cppflags  --ldflags --libs` obj/rigolscope.cpp.o obj/plotting.cpp.o -o rigolscope

obj/%.cpp.o: src/%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@


clean_all: clean
	rm rigolserv
	rm rigolscope

clean:
	rm -rf obj/*.o

