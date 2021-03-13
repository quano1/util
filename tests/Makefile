# Targets
.PHONY: default clean all test_utils.out

NUM_CPU=`grep -c ^processor /proc/cpuinfo`
CXX=g++
LD=g++
CXX_FLAGS = -std=c++17 -fopenmp -O3 -DNUM_CPU=$(NUM_CPU) -g
LD_FLAGS = -lpthread -fopenmp

default: all

all: test_utils.out

test_utils.o: test_utils.cc
	$(CXX) -c $(CXX_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" $< -o $@


test_utils.out: test_utils.o
	$(LD) $(LD_FLAGS) $? -o $@
	objcopy --only-keep-debug $@ $(@:%.out=%.debug)
	strip -g $@

perfTunnel.dat: test_utils.out
	@echo "Running test_utils..."
	./$< > run.log

clean:
	rm -f *.o *.d *.out *.debug

# g++ test_utils.cc -std=c++17 -lpthread -fopenmp -O3 -D NUM_CPU=$(grep -c ^processor /proc/cpuinfo)

plot: perfTunnel.dat
	gnuplot -c perfTunnel.p $< $(@:%.dat=%.png) $(NUM_CPU)

-include test_utils.d