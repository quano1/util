# Targets
.PHONY: default clean all test_cc.out benchmark.out

NUM_CPU=`grep -c ^processor /proc/cpuinfo`
CXX=g++
LD=g++
CXX_FLAGS = -std=c++17 -fopenmp -O3 -DNUM_CPU=$(NUM_CPU) -g
LD_FLAGS = -lpthread -fopenmp

default: all

all: test_cc.out benchmark.out

test_cc.o: test_cc.cc
	$(CXX) -c $(CXX_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" $< -o $@

benchmark.o: benchmark.cc
	$(CXX) -c $(CXX_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" $< -o $@

test_cc.out: test_cc.o
	$(LD) $(LD_FLAGS) $? -o $@
	objcopy --only-keep-debug $@ $(@:%.out=%.debug)
	strip -g $@

benchmark.out: benchmark.o
	$(LD) $(LD_FLAGS) $? -o $@ -ltbb
	objcopy --only-keep-debug $@ $(@:%.out=%.debug)
	strip -g $@

benchmark.dat: benchmark.out
	@echo "Running benchmark..."
	./$<

clean:
	rm -f *.o *.d *.out *.debug

# g++ test_cc.cc -std=c++17 -lpthread -fopenmp -O3 -D NUM_CPU=$(grep -c ^processor /proc/cpuinfo)

plot: benchmark.dat
	gnuplot -c perfTunnel.p $< $(<:%.dat=%.png) $(NUM_CPU)

-include test_cc.d benchmark.d