#!/usr/bin/gnuplot --persist


# THIRD=ARG3
# print "script name        : ", ARG0
# print "first argument     : ", ARG1
# print "third argument     : ", THIRD 
# print "number of arguments: ", ARGC 

set terminal png size 1280, 800
set output sprintf('%s', ARG2)

set title sprintf("push&pop 1 byte 10 million times\nMax CPU %s", ARG3)
set ylabel "Ops(million)/second"
set xlabel "Threads"


# set logscale x
# set xtics scale 0
# set xtics scale 0 format ""
# map(x) = x
# inv_map(x) = x

# set nonlinear x via map(x) inverse inv_map(x)

f(x) = x <= ARG3 ? x : log(x)/log(2) + ARG3 - 2
g(x) = x <= ARG3 ? x : 2**(x - ARG3 + 2)
set nonlinear x via f(x) inv g(x)

# set x2tics
set y2tics

# set key reverse Left outside
set grid

# set samples 1000


set style line 1 lc rgb '#ffffbf' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 2 lc rgb '#cacc5d' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 3 lc rgb '#ff6434' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 4 lc rgb '#a30000' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 5 lc rgb '#7a7cff' pt 1 ps 1 lt 1 lw 2 # --- 
set style line 6 lc rgb '#0026ca' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 7 lc rgb '#9cff57' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 8 lc rgb '#1faa00' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 9 lc rgb '#6a4f4b' pt 6 ps 1 lt 1 lw 2 # --- 
set style line 10 lc rgb '#1b0000' pt 6 ps 1 lt 1 lw 2 # --- 

plot sprintf('%s', ARG1) u 1:2:xtic(1) t "CCFIFO push" w lp ls 1, \
      ""      u 1:3:xtic(1) t "CCFIFO pop" w lp ls 2, \
      ""      u 1:4:xtic(1) t "CCFIFO enQ" w lp ls 3, \
      ""      u 1:5:xtic(1) t "CCFIFO deQ" w lp ls 4, \
      ""      u 1:6:xtic(1) t "Boost push" w lp ls 5, \
      ""      u 1:7:xtic(1) t "Boost pop" w lp ls 6, \
      ""      u 1:8:xtic(1) t "tbb push" w lp ls 7, \
      ""      u 1:9:xtic(1) t "tbb pop" w lp ls 8, \
      ""      u 1:10:xtic(1) t "moodycamel push" w lp ls 9, \
      ""      u 1:11:xtic(1) t "moodycamel pop" w lp ls 10

