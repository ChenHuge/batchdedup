CXX := g++

.PHONY: all
all: batchsim

batchsim: batchsim.o BackupScheduler.o
	$(CXX) -o $@ $^

%.o: %.cpp %.h
	$(CXX) -c -o $@ $<

%.o: %.cpp
	$(CXX) -c -o $@ $<

.PHONY: clean
clean:
	rm -f batchsim *.o
