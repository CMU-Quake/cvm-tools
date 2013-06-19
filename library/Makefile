# -*- Makefile -*-
#
# Set the following flag to mach the current platform
# from the following set of values: intel, alpha, sun, or mac
#
# Note: Mac OS X does not support shared library. You need to link your code with the 
#       static lirary libetree.a.
#

PLATFORM = intel

# OPTIMIZEFLAG: set to -g or -O2 depending on your need.
OPTIMIZEFLAG = -g

-include user.make

# set PLATFORM_FLAGS depending on the platform
ifeq ($(PLATFORM),intel)
	PLATFORM_FLAGS = -DINTEL 
endif

ifeq ($(PLATFORM),sun)
	PLATFORM_FLAGS = -DSOLARIS -DALIGNMENT
	LD_FLAGS = -lsocket
endif

ifeq ($(PLATFORM),alpha)
	PLATFORM_FLAGS = -DALPHA -DALIGNMENT
endif

ifeq ($(PLATFORM),mac) 
	PLATFORM_FLAGS = 
endif


ETREE_DIR = ..

#
# We use GNU C compiler to build the library
#
CC = gcc
LD = gcc
TARGET = libetree.a 

#
# The -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 flags are
# necessary to enable large file system support	
#
CFLAGS = $(OPTIMIZEFLAG) \
	-Wall \
	-fPIC \
	-I$(ETREE_DIR) \
	-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	$(PLATFORM_FLAGS)

# Create a dynamic library unless compiling on Mac OS X
ifneq ($(PLATFORM),mac) 
	TARGET += libetree.so
endif


#
# Object modules of the library
#
OBJECTS = dlink.o code.o buffer.o schema.o xplatform.o btree.o etree.o \
	wrapper.o schemax.o

all: $(TARGET)

libetree.a: $(OBJECTS)
	ar rcs libetree.a $(OBJECTS) 

libetree.so: $(OBJECTS)
	$(LD) -shared $(LD_FLAGS) -o libetree.so $(OBJECTS)

clean:
	rm -f $(OBJECTS) *~

cleanall:
	rm -f $(TARGET) $(OBJECTS) *~	
