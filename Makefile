# Makefile for thread library project

CXX = g++
CXXFLAGS = -Wall -Werror -std=c++11 -g -ldl
ifeq ($(shell uname), Darwin)
LIBS = libinterrupt-mac.a
CLEANCMD = find . -type f -perm +ugo=x -delete
CXXFLAGS := $(CXXFLAGS) -D_XOPEN_SOURCE -Wno-deprecated-declarations
else
LIBS = libinterrupt.a
CLEANCMD = find . -type f -executable -delete
endif

default:
	@echo "Specify a test program (e.g., make sample to build sample.cc)"

% : %.cc thread.cc
	$(CXX) $(CXXFLAGS) -o $@ $@.cc thread.cc $(LIBS)

clean:
	$(CLEANCMD)
	rm -rf *.dSYM

