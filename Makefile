#ifndef WORKDIR
	WORKDIR = $(PWD)
#endif

include $(WORKDIR)/system.mk
include $(WORKDIR)/common.mk

CFLAGS += -I$(ETREE_DIR) 
LOADLIBES += $(ETREE_DIR)/libetree.a

# Object modules 

OBJECTS = cvm.o .setdbctl.o showdbctl.o

TARGET = showdbctl querycvm querymesh scancvm dumpcvm pickrecord asciivol lltoxy mirrorkims mirrorrobs setappmeta

.PHONY: all clean cleanall etree cvmtools 

all: etree cvmtools

etree:
	$(MAKE) -C $(ETREE_DIR) SYSTEM=$(SYSTEM) WORKDIR=$(WORKDIR)

cvmtools: $(TARGET)

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
	$(MAKE) -C $(ETREE_DIR) WORKDIR=$(WORKDIR) clean
	rm -f $(OBJECTS) core *.o *~

cleanall:
	$(MAKE) -C $(ETREE_DIR) WORKDIR=$(WORKDIR) cleanall
	rm -f $(OBJECTS) $(TARGET) core *.o *~


