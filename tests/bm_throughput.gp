#!/usr/bin/gnuplot --persist


# THIRD=ARG3
# print "script name        : ", ARG0
# print "first argument     : ", ARG1
# print "third argument     : ", THIRD 
# print "number of arguments: ", ARGC 

set terminal png size 1280, 800
set output sprintf('%s', ARG2)

set title sprintf("%s 1 byte contigously\nLower is better", ARG4)
set ylabel "Avg time of one operation (us)"
set xlabel sprintf("Number of threads (Max CPU: %s)", ARG3)

# set logscale x
# set xtics scale 0
# set xtics scale 0 format ""
# map(x) = x
# inv_map(x) = x

# set nonlinear x via map(x) inverse inv_map(x)

arg3 = ARG3

f(x) = (x == 1) ? 1. : (x == arg3/2) ? 2. : (x == arg3) ? 3. : log(x/arg3)/log(2) + 3
g(x) = (x == 1) ? 1. : (x == 2) ? (arg3/2) : (x == 3) ? arg3 : ((2**(x - 3)) * arg3)
# arg3 = ARG3 / 2
# f(x) = x < arg3 ? x : x == 4 ? 3. : log(x)/log(2) + arg3 - 2
# g(x) = x <= arg3 ? x : 2**(x - arg3 + 2)
# f(x) = x == 1 ? 0 : x == 2 ? 1 : x == 4 ? 2 : x == 8 ? 3 : 4
# g(x) = x == 0 ? 1 : x == 1 ? 2 : x == 2 ? 4 : x == 3 ? 8 : 16

# 2**(x - arg3 + 2)
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

if(ARG4 eq "push") {
    plot sprintf('%s', ARG1) u 1:2:xtic(1) t "CCFIFO" w lp ls 1, \
          ""      u 1:4:xtic(1) t "boost" w lp ls 5, \
          ""      u 1:6:xtic(1) t "moodycamel" w lp ls 7

          # ""      u 1:6:xtic(1) t "tbb" w lp ls 7,
}
else {
    plot sprintf('%s', ARG1) u 1:3:xtic(1) t "CCFIFO" w lp ls 1, \
          ""      u 1:5:xtic(1) t "boost" w lp ls 5, \
          ""      u 1:7:xtic(1) t "moodycamel" w lp ls 7

          # ""      u 1:7:xtic(1) t "tbb" w lp ls 5,
}

