INCDIR = ../include
LIBDIR = ../lib

ODIR = ../build

CXX = g++
CXXFLAGS = -fPIC -Wall `pkg-config --cflags opencv` -I$(INCDIR)
LDFLAGS = -Wl,-rpath,$(LIBDIR) -L$(LIBDIR)
LDLIBS = -lmysqlclient -limgen `pkg-config --libs opencv`

PROG = vdgen
LIB =

MAINOBJ = $(ODIR)/main.o
SQLOBJ = $(ODIR)/MySQL_API.o 
LIBOBJS =

all: $(PROG) $(LIB)

$(PROG):$(MAINOBJ) $(LIBOBJS) $(SQLOBJ)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(LIB):$(LIBOBJS) $(SQLOBJ)
	$(CXX) $^ -o $@ -shared

$(ODIR)/%.o:$(addprefix ./, %.cpp)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY:clean

clean:
	rm $(ODIR)/*.o vdgen 
