CC = mpicc
CXX = mpicxx
LDLIBS = -lssl
CPPFLAGS = -g -Wall -DDEBUG -wd981
#CPPFLAGS = -O2 -Wall -DNDEBUG

BINS = batch_dedup
DEPS = disk_io mpi_engine snapshot_mixer batch_dedup_config snapshot_meta data_spout data_sink dedup_buffer partition_index trace_types
OBJS = $(addsuffix .o,$(BINS))
DEP_OBJS = $(addsuffix .o,$(DEPS))
DEP_HDRS = $(addsuffix .h,$(DEPS))

all: $(BINS)

$(BINS): $(OBJS) $(DEP_OBJS)

$(BINS) $(DEPS): %:%.cc

$(DEPS): %:%.h

clean:
	rm -f $(BINS) *.o
