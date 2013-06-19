#ifndef WORKDIR
	WORKDIR = $(PWD)/..
#endif

# Compiler program

-include user.make

CC = gcc
LD = gcc

# Etree directory and library

ETREE_DIR = $(CURDIR)/library
ETREE_LIB = $(ETREE_DIR)/libetree.so

CVM_DIR = $(WORKDIR)/cvm

#CFLAGS += -DCVM_SRCPATH=\"$(WORKDIR)/results/current.e\"
#CFLAGS += -DCVM_DESTDIR=\"$(WORKDIR)/results\"

CFLAGS += -O2 \
	  -Wall \
	  -fPIC \
	  -I$(ETREE_DIR) \
	  -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	  -DINTEL

LDFLAGS += -lm

# Compiler flags
#CFLAGS = -g -Wall -I $(ETREE_DIR) $(ALPHAFLAGS) $(PERFFLAGS)
#CFLAGS = -O2 -Wall -I $(ETREE_DIR) $(ALPHAFLAGS) $(PERFLAGS)

# include $(WORKDIR)/systemdef.mk
# include $(WORKDIR)/common.mk

# CFLAGS += -I$(ETREE_DIR) 

LOADLIBES += $(ETREE_DIR)/libetree.so

#
# Object modules 
#

OBJECTS = .setdbctl.o cvm.o showdbctl.o

TARGET = showdbctl querycvm querymesh scancvm dumpcvm pickrecord asciivol lltoxy mirrorkims mirrorrobs setappmeta
#TARGET = querycvm

all: $(TARGET)

.setdbctl: cvm.o .setdbctl.o 
showdbctl: cvm.o showdbctl.o 
querycvm: cvm.o querycvm.o 
querymesh: cvm.o querymesh.o
scancvm: scancvm.o
dumpcvm: dumpcvm.o 
pickrecord: pickrecord.o 
asciivol: cvm.o asciivol.o
lltoxy: lltoxy.o
mirrorkims: mirrorkims.o
mirrorrob: mirrorrobs.o
setappmeta: cvm.o setappmeta.o

clean:
	rm -f $(OBJECTS) core *.o *~

cleanall:
	rm -f $(OBJECTS) $(TARGET) core *~
