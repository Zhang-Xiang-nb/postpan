# Regression/Dithering Analyzer redana
#  - B. Moffit and K. Paschke
# 
# Uncomment this if profiling (note: it slows down the code)
# export PROFILE = 1
#
# To compile optimized  (10% faster run time but very slow compilation)
# export OPTIMIZE =1
# 

# Choose the compiler.
GCC=$(shell root-config --cxx)
GLD=$(shell root-config --cc)

ifdef INSURE
  GCC=insure
  GLD=insure
endif

ROOTLIBS      = $(shell root-config --libs)
ROOTGLIBS     = $(shell root-config --glibs)
INCLUDES      = -I$(shell root-config --incdir)
CXX           = $(GCC)
CXXFLAGS      = -Wall -fno-exceptions -fPIC -fpermissive
CXXFLAGS      += $(shell root-config --cflags)
LD            = $(GLD)
LDFLAGS       = 
SOFLAGS       = -shared 
GLIBS         = $(ROOTGLIBS) -lstdc++
MAKEDEPEND    = $(GCC)

ALL_LIBS = $(GLIBS)


ifdef PROFILE
   CXXFLAGS += -pg
endif

   CXXFLAGS += -O2
   CXXFLAGS += -g


# Grow the list of sources and pan objects here.

SRC = 	TaRedAna.cc TaConfFile.cc VaAna.cc TaRegression.cc TaInput.cc \
	TaDitAna.cc TaDitCyc.cc Matrix.cc TaFileName.cc TaString.cc\
	TaStatistics.cc TaOResultsFile.cc
DEPS = $(SRC:.cc=.d)
OBJS = $(SRC:.cc=.o) 
DOBJS = $(SRC:.cc=.o)
HEAD = $(SRC:.cc=.hh) 

# Make the dictionary
  CXXFLAGS += -DDICT
  OBJS += RedDict.o

PROGS = redana

# install: all
# 	 mv $(PROGS) ../
# 	 mv libRedAna.so ../

all: $(PROGS) libRedAna.so

$(PROGS): main.o $(OBJS) $(SRC) $(HEAD) 
	rm -f $@
	@echo "Generating" $@
	$(LD) $(CXXFLAGS) -o $@ main.o $(OBJS) $(ALL_LIBS) 

main.o: main.cc
	@echo "Generating" $@
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $<	

libRedAna.so: $(OBJS) $(HEAD)
	@echo "Generating" $@
	$(CXX) $(SOFLAGS) -O2 -o libRedAna.so $(OBJS) $(ALL_LIBS)

# Dictionary
RedDict.cc: $(DOBJS) RedLinkDef.hh
	@echo "Generating Dictionary..."
	$(ROOTSYS)/bin/rootcint -f $@ -c -p -DDICT $(HEAD) RedLinkDef.hh

clean:
	rm -f *.o core *~ *.d *.tar *.so  *Dict* $(PROGS)

realclean:  clean
	rm -f *.d ../$(PROGS) ../libRedAna.so

.SUFFIXES:
.SUFFIXES: .c .cc .cpp .C .o .d

%.o:	%.cc
	$(CXX) $(CXXFLAGS) -c $<

%.d:	%.cc
	@echo Creating dependencies for $<
	@$(SHELL) -ec '$(MAKEDEPEND) -MM $(INCLUDES) -c $< \
	      | sed '\''s%^.*\.o%$*\.o%g'\'' \
	      | sed '\''s%\($*\)\.o[ :]*%\1.o $@ : %g'\'' > $@; \
	      [ -s $@ ] || rm -f $@'

-include $(DEPS)


