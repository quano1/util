# Targets
.PHONY: default clean all test_cc.out benchmark.out

NUM_CPU=`grep -c ^processor /proc/cpuinfo`
CXX=g++
LD=g++
CXX_FLAGS = -std=c++17 -fopenmp -O3 -DNUM_CPU=$(NUM_CPU) -g -rdynamic
LD_FLAGS = -lpthread -fopenmp

default: all

all: test_cc.out benchmark.out perf_tunnel.out

test_cc.o: test_cc.cc
	$(CXX) -c $(CXX_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" $< -o $@

benchmark.o: benchmark.cc
	$(CXX) -c $(CXX_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" $< -o $@

perf_tunnel.o: perf_tunnel.cc
	$(CXX) -c $(CXX_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" $< -o $@

test_cc.out: test_cc.o
	$(LD) $(LD_FLAGS) $? -o $@
	objcopy --only-keep-debug $@ $(@:%.out=%.debug)
# 	strip -g $@

benchmark.out: benchmark.o
	$(LD) $(LD_FLAGS) $? -o $@ -ltbb
	objcopy --only-keep-debug $@ $(@:%.out=%.debug)
# 	strip -g $@

perf_tunnel.out: perf_tunnel.o
	$(LD) $(LD_FLAGS) $? -o $@ -ltbb
	objcopy --only-keep-debug $@ $(@:%.out=%.debug)

bm_throughput.dat: benchmark.out
	@echo "Running benchmark..."
	./$<

clean:
	rm -f *.o *.d *.out *.debug *.dat *.log

# g++ test_cc.cc -std=c++17 -lpthread -fopenmp -O3 -D NUM_CPU=$(grep -c ^processor /proc/cpuinfo)

plot: bm_throughput.dat
	mkdir -p tmp
	gnuplot -c bm_throughput.gp bm_throughput.dat tmp/push_`date +'%y%m%d_%H%M'`.png $(NUM_CPU) 'push'
	gnuplot -c bm_throughput.gp bm_throughput.dat tmp/pop_`date +'%y%m%d_%H%M'`.png $(NUM_CPU) 'pop'
	gnuplot -c bm_time.gp bm_time.dat tmp/pp_`date +'%y%m%d_%H%M'`.png $(NUM_CPU)

-include test_cc.d benchmark.d perf_tunnel.d