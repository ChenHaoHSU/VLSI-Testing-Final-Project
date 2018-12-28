
BINNAME   = atpg
BINPATH   = bin/
SRCPATH   = src/
EXE       = $(BINPATH)$(BINNAME)

CXX       = g++
CHDRS     = $(wildcard $(SRCPATH)*.h)
CSRCS     = $(wildcard $(SRCPATH)*.cpp)
COBJS     = $(addsuffix .o, $(basename $(CSRCS)))

CFLAGS    = -std=c++11 -g -Wall -static
CFLAGS    = -std=c++11 -O3 -Wall -static

ECHO      = echo
RM        = rm
MKDIR     = mkdir

$(EXE): $(COBJS)
	@$(MKDIR) -p $(BINPATH) 
	@$(ECHO) "[Build Target] $(EXE)"
	@$(CXX) $(CFLAGS) $(COBJS) -lm -o $(EXE)

$(SRCPATH)%.o: $(SRCPATH)%.cpp $(CHDRS)
	@$(ECHO) "[Compile] $<"
	@$(CXX) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) -rf $(EXE) $(COBJS)
