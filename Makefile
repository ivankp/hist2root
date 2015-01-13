#################################################
# Developed by Ivan Pogrebnyak
# MSU 2015
#################################################

CPP := g++

CFLAGS := -Wall -g -Isrc

ROOT_CFLAGS := $(shell root-config --cflags)
ROOT_LIBS   := $(shell root-config --libs)

.PHONY: all clean

EXE = hist2root

all: $(EXE)

$(EXE): %: %.cc
	$(CPP) $(CFLAGS) $(ROOT_CFLAGS) $^ -o $@ $(ROOT_LIBS)

clean:
	rm -rf $(EXE)
