CXX := g++

.PHONY: all
all: batchsim wlgen

.PHONY: debug
debug: batchsim-debug

batchsim-debug: batchsim.cpp BackupScheduler.cpp BackupScheduler.h
	$(CXX) -o $@ batchsim.cpp BackupScheduler.cpp -g

batchsim: batchsim.o BackupScheduler.o
	$(CXX) -o $@ $^

wlgen: wlgen.o WLGenerator.o
	$(CXX) -o $@ $^

%.o: %.cpp %.h
	$(CXX) -c -o $@ $<

%.o: %.cpp
	$(CXX) -c -o $@ $<

.PHONY: clean
clean:
	rm -f batchsim *.o
