set terminal png size 1920, 1080;
set output 'perfTunnel.png';

set title "performance tunnel"
set ylabel "operations per second"
set xlabel "num of threads"

set key reverse Left outside
set grid

set samples 1000

set style line 1 lc rgb '#8b1a0e' pt 1 ps 1 lt 1 lw 2 # --- red
set style line 2 lc rgb '#5e9c36' pt 6 ps 1 lt 1 lw 2 # --- green

plot "perfTunnel.dat" u 1:2 t "Enqueue" w lp ls 1, \
      ""      u 1:3 t "Dequeue" w lp ls 2

