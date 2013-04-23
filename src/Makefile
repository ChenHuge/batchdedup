CC = mpicc
CXX = mpicxx
LDLIBS = -lssl
CPPFLAGS = -g -Wall -DDEBUG -wd981
#CPPFLAGS = -O2 -Wall -DNDEBUG

BINS = batch_dedup
DEPS = mpi_engine snapshot_mixer batch_dedup_config snapshot_meta data_spout data_sink dedup_buffer partition_index trace_types
BIN_OBJS = $(addsuffix .o,$(BINS))
DEP_OBJS = $(addsuffix .o,$(DEPS))
DEP_HDRS = $(addsuffix .h,$(DEPS))

all: $(BINS)

$(BINS): $(BIN_OBJS) $(DEP_OBJS)

$(BIN_OBJS) $(DEP_OBJS): 
#%:%.cc

$(DEPS): %:%.h

dedup_buffer.o: disk_io.h

clean:
	rm -f $(BINS) *.o
