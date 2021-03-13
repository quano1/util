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

set style line 1 lc rgb '#7a7cff' pt 1 ps 1 lt 1 lw 2 # --- 
set style line 2 lc rgb '#0026ca' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 3 lc rgb '#ff6434' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 4 lc rgb '#a30000' pt 6 ps 1 lt 1 lw 2 # --- 

plot sprintf('%s', ARG1) u 1:2:xtic(1) t "CCFIFO En" w lp ls 1, \
      ""      u 1:3:xtic(1) t "CCFIFO De" w lp ls 2, \
      ""      u 1:4:xtic(1) t "Boost En" w lp ls 3, \
      ""      u 1:5:xtic(1) t "Boost De" w lp ls 4

