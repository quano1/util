#!/usr/bin/gnuplot --persist


# THIRD=ARG3
# print "script name        : ", ARG0
# print "first argument     : ", ARG1
# print "third argument     : ", THIRD 
# print "number of arguments: ", ARGC 

set terminal png size 1280, 800
set output sprintf('%s', ARG2)

set title sprintf("push pop 1 byte 10 million times\nMax CPU %s", ARG3)
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


set style line 1 lc rgb '#512da8' pt 6 ps 1 lt 1 lw 2
set style line 2 lc rgb '#8559da' pt 6 ps 1 lt 1 lw 2
set style line 3 lc rgb '#d32f2f' pt 6 ps 1 lt 1 lw 2
set style line 4 lc rgb '#ff6659' pt 6 ps 1 lt 1 lw 2
set style line 5 lc rgb '#0288d1' pt 1 ps 1 lt 1 lw 2
set style line 6 lc rgb '#5eb8ff' pt 6 ps 1 lt 1 lw 2
set style line 7 lc rgb '#689f38' pt 6 ps 1 lt 1 lw 2
set style line 8 lc rgb '#99d066' pt 6 ps 1 lt 1 lw 2
set style line 9 lc rgb '#f57c00' pt 6 ps 1 lt 1 lw 2
set style line 10 lc rgb '#ffad42' pt 6 ps 1 lt 1 lw 2

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

