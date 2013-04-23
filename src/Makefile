CC = mpicc
CXX = mpicxx
LDLIBS = -lssl -lstdc++
CPPFLAGS = -g -Wall -DDEBUG -wd981 -wd869 -wd1418 -wd383
#CPPFLAGS = -g -Wall -DDEBUG
#CPPFLAGS = -O2 -Wall -DNDEBUG

BINS = batch_dedup
DEPS = mpi_engine snapshot_mixer data_spout data_sink dedup_buffer partition_index snapshot_meta batch_dedup_config trace_types
BIN_OBJS = $(addsuffix .o,$(BINS))
DEP_OBJS = $(addsuffix .o,$(DEPS))
DEP_HDRS = $(addsuffix .h,$(DEPS))

all: $(BINS)

$(BINS): $(DEP_OBJS) $(BIN_OBJS)

$(BIN_OBJS) $(DEP_OBJS): 
#%:%.cc

$(DEPS): %:%.h

dedup_buffer.o: disk_io.h

clean:
	rm -f $(BINS) *.o
