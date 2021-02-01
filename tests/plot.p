set terminal png size 800,600;
set output 'xyz.png';

set title "update every kth element"
set ylabel "time in s"
set xlabel "Mb"
set logscale x
# set logscale y
set key reverse Left outside
set grid
set style data linespoints
# plot "ls.dat" using 1:2:xtic(1) title "cache access 1", "" using 1:3:xtic(1) title "cache access 2"

plot "run.dat" using 1:2:xtic(1) title "mutex", \
      ""      using 1:3:xtic(1) title "lf 1t", \
      ""      using 1:4:xtic(1) title "lf 2t"
      # ""      using (log($1)):5:xtic(1) title "lf 3t"
# :(sprintf("[%.6f]",$4)) with labels