# CXXFLAGS += -DSPANDSP_VER3

LDFLAGS	=-L /usr/local/lib -lspandsp
CXXFLAGS  += -g
#CXXFLAGS  += -DSPANDSP_VER3
LD	= g++

spandsp_util:	spandsp_util.o spandsp_if.o
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ spandsp_util.o spandsp_if.o

spandsp_util.o:	spandsp_util.cpp

spandsp_if.o:	spandsp_if.cpp

clean:	
	rm -f spandsp_util spandsp_util.o spandsp_if.o


