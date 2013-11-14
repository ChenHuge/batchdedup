CXX := g++

.PHONY: all
all: batchsim wlgen

.PHONY: debug
debug: batchsim-debug

batchsim-debug: batchsim.cc ../src/RoundScheduler.cc ../src/RoundScheduler.h
	$(CXX) -o $@ batchsim.cc ../src/RoundScheduler.cc -g

batchsim: batchsim.o RoundScheduler.o
	$(CXX) -o $@ $^

wlgen: wlgen.o WLGenerator.o
	$(CXX) -o $@ $^

RoundScheduler.o: ../src/RoundScheduler.cc ../src/RoundScheduler.h
	$(CXX) -c -o $@ $<

%.o: %.cc %.h
	$(CXX) -c -o $@ $<

%.o: %.cc
	$(CXX) -c -o $@ $<

.PHONY: clean
clean:
	rm -f wlgen batchsim *.o
