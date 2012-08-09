


CXX = g++

CFLAGS = -g -O3 -I/usr/local/include/GraphicsMagick -I/usr/local/include/libusb-1.0
LIBS = 

SCOPESERV_OBJS = obj/rigolserv.cpp.o
SCOPESERV_OBJS += obj/freetmc_local.cpp.o

SCOPECMD_OBJS = obj/scopecmd.cpp.o
SCOPECMD_OBJS += obj/freetmc_local.cpp.o

SCOPEV_OBJS = obj/scopev.cpp.o
SCOPEV_OBJS += obj/plotting.cpp.o
SCOPEV_OBJS += obj/freetmc_local.cpp.o


ifeq ($(shell uname -s), Darwin)
	CFLAGS += -DMACOSX -L/usr/local/lib
	OGLFLAGS = -framework GLUT -framework OpenGL -framework Cocoa
else
	CFLAGS += -DLINUX
	OGLFLAGS = -lglut -lgl -lglu
endif

L_LIBUSB = -L/usr/local/lib -lusb-1.0


all: scopeserv scopev scopecmd

obj:
	mkdir -p obj

scopeserv: obj src/simple_except.h src/freetmc.h src/netcomm.h ${SCOPESERV_OBJS}
	$(CXX) $(CFLAGS) ${L_LIBUSB} ${SCOPESERV_OBJS} -o scopeserv

scopecmd: obj src/simple_except.h src/freetmc.h src/netcomm.h src/remotedevice.h src/rigoltmc.h ${SCOPECMD_OBJS}
	$(CXX) $(CFLAGS) ${L_LIBUSB} ${SCOPECMD_OBJS} -o scopecmd

scopev: obj src/simple_except.h src/freetmc.h src/netcomm.h src/remotedevice.h src/rigoltmc.h ${SCOPEV_OBJS}
	$(CXX) $(CFLAGS) ${OGLFLAGS} ${L_LIBUSB} `GraphicsMagick++-config --cxxflags --cppflags  --ldflags --libs` ${SCOPEV_OBJS} -o scopev

obj/%.cpp.o: src/%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@


clean_all: clean
	rm rigolserv
	rm rigolscope

clean:
	rm -rf obj/*.o

