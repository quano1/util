#!/usr/bin/gnuplot --persist


# THIRD=ARG3
# print "script name        : ", ARG0
# print "first argument     : ", ARG1
# print "third argument     : ", THIRD 
# print "number of arguments: ", ARGC 

set terminal png size 1280, 800
set output sprintf('%s', ARG2)

set title sprintf('max cpu: %s', ARG3)
set ylabel "operations per second"
set xlabel "num of threads"

set key reverse Left outside
set grid

# set samples 1000

set style line 1 lc rgb '#8b1a0e' pt 1 ps 1 lt 1 lw 2 # --- red
set style line 2 lc rgb '#5e9c36' pt 6 ps 1 lt 1 lw 2 # --- green

plot sprintf('%s', ARG1) u 1:2:xtic(1) t "Enqueue" w lp ls 1, \
      ""      u 1:3:xtic(1) t "Dequeue" w lp ls 2

