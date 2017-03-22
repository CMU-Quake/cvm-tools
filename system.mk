# -*- Makefile -*-
#
# Override SYSTEM and other variable definition in user.mk
#
# In order to switch between a debug and an optimized executable, set CFLAGS
# in your 'user.mk' file as follows:
#
# * For an executable with debug information:
#	CFLAGS = -g -DDEBUG -O0
#
# * For an optimized executable:
#	CFLAGS = -O2
#
# check other platform specific flags below.
#

-include $(WORKDIR)/user.mk

ifndef SYSTEM
	SYSTEM = CRAY
endif

ifeq ($(SYSTEM), CRAY)
        CC      = cc
        CXX     = CC
        LD      = CC
        CFLAGS  += -DBIGBEN
        LDFLAGS +=
        ifdef IOBUF_INC
            CPPFLAGS += -I${IOBUF_INC}
        endif
        CPPFLAGS    += -D_USE_FILE_OFFSET64 -D_FILE_OFFSET_BITS=64 -D_USE_LARGEFILE64
endif

ifeq ($(SYSTEM), MACBOOK)
	MPI_DIR      = /usr/
        MPI_INCLUDE  = $(MPI_DIR)/include/openmpi/ompi/mpi/cxx
        CC           = $(MPI_DIR)/bin/mpicc
        CXX          = $(MPI_DIR)/bin/mpicxx
        LD           = $(MPI_DIR)/bin/mpicxx
        CXXFLAGS    += -DMPICH_IGNORE_CXX_SEEK
        CFLAGS      += -Wall
        CPPFLAGS    += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
endif

ifeq ($(SYSTEM), MAVERICKS)
	MPI_DIR      = /opt/local/
	MPI_INCLUDE  = $(MPI_DIR)/include/openmpi-mp/openmpi/ompi/mpi/cxx
	CC           = $(MPI_DIR)/bin/mpicc-openmpi-mp
	CXX          = $(MPI_DIR)/bin/mpicxx-openmpi-mp
	LD           = $(MPI_DIR)/bin/mpicxx-openmpi-mp
	CXXFLAGS    += -DMPICH_IGNORE_CXX_SEEK
	CFLAGS      += -Wall
	CPPFLAGS    += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
endif
