#!/usr/bin/gnuplot --persist


# THIRD=ARG3
# print "script name        : ", ARG0
# print "first argument     : ", ARG1
# print "third argument     : ", THIRD 
# print "number of arguments: ", ARGC 

set terminal png size 1280, 800
set output sprintf('%s', ARG2)

set title sprintf("push/pop 1 byte 10 million times\nLower is better")
set ylabel "Time Of Execution (second)"
set xlabel sprintf("Number Of Threads/2 (Max CPU: %s)", ARG3)

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


set style line 1 lc rgb '#d32f2f' pt 6 ps 1 lt 1 lw 3
set style line 2 lc rgb '#ff6659' pt 6 ps 1 lt 1 lw 2
set style line 3 lc rgb '#512da8' pt 6 ps 1 lt 1 lw 2
set style line 4 lc rgb '#8559da' pt 6 ps 1 lt 1 lw 2
set style line 5 lc rgb '#0288d1' pt 1 ps 1 lt 1 lw 2
set style line 6 lc rgb '#5eb8ff' pt 6 ps 1 lt 1 lw 2
set style line 7 lc rgb '#689f38' pt 6 ps 1 lt 1 lw 2
set style line 8 lc rgb '#99d066' pt 6 ps 1 lt 1 lw 2
set style line 9 lc rgb '#f57c00' pt 6 ps 1 lt 1 lw 2
set style line 10 lc rgb '#ffad42' pt 6 ps 1 lt 1 lw 2

plot sprintf('%s', ARG1) u 1:2:xtic(1) t "CCFIFO" w lp ls 1, \
      ""      u 1:3:xtic(1) t "boost" w lp ls 5, \
      ""      u 1:4:xtic(1) t "moodycamel" w lp ls 7

      # ""      u 1:5:xtic(1) t "tbb" w lp ls 7
