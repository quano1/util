#!/bin/bash
mkdir -p tmp
dat_file=`echo ${1} | sed -e '/\//{ s/.*\/\([^.]*\)\.dat/\1/g; q; }' -e 's/\([^\.]*\)\.dat/\1/;'`;
echo ${dat_file}
# output=tmp/${dat_file}.png
output=tmp/${dat_file}.`date +'%y%m%d_%H%M'`.png
# echo $dat_file;
# exit 1;
dat_file=.${dat_file}.dat
rm -f ${dat_file}
>${dat_file}

declare -a texts

while IFS= read -r line
do
    line2=`echo "$line" | sed -n -e '/^\;/{ s/^.//; p; }'`;
    [[ ! -z $line2 ]] && {
        texts+=("${line2}");
    } || {
        echo $line >> ${dat_file};
    }
done < $1

num_cols=${#texts[@]};
echo "Number of columns: $((num_cols-4))";
[[ ${#texts[@]} -lt 5 ]] && { echo "something wrong"; exit 1; };

num_cpus=${texts[0]}
title=${texts[1]}
yasix=${texts[2]}
xasix=${texts[3]}

[[ -z ${num_cpus} ]] && { echo "cpus == 0"; exit 1; };

rm -f .plot.gpl;
> .plot.gpl;

echo "set terminal png size 1280, 800" >> .plot.gpl;
echo "set output \"$output\"" >> .plot.gpl;
echo "set title \"$title\"" >> .plot.gpl;
echo "set ylabel \"$xasix\"" >> .plot.gpl;
echo "set xlabel \"$yasix\"" >> .plot.gpl;
echo "set y2tics" >> .plot.gpl;
echo "set grid" >> .plot.gpl;
echo "set key right bottom" >> .plot.gpl;

echo "num_cpus = $num_cpus" >> .plot.gpl;

[[ $# > 1 ]] && {
    echo "Scale enabled"
    echo "f(x) = (x == 1) ? 0. : (x == num_cpus/2) ? 1. : x/num_cpus + 1" >> .plot.gpl;
    echo "g(x) = (x == 0) ? 1. : (x == 1) ? (num_cpus/2) : (x - 1) * num_cpus" >> .plot.gpl;
    echo "set nonlinear x via f(x) inv g(x)" >> .plot.gpl;
}

offset=2;
index=2;
echo "plot \"${dat_file}\"    u 1:${index}:xtic(1) t \"${texts[$((index+offset))]}\" w lp pt 6 lw 2, \\" >> .plot.gpl;
index=3;
while true; do
    # echo $index;
    echo "    \"\"    u 1:${index}:xtic(1) t \"${texts[$((index+offset))]}\" w lp pt 6 lw 2, \\" >> .plot.gpl;
    index=$((index+1));
    echo "index: $((index+offset))";
    [[ $((index+offset)) == $((num_cols-1)) ]] && break;
    # break;
done
echo "    \"\"    u 1:${index}:xtic(1) t \"${texts[$((index+offset))]}\" w lp pt 6 lw 2" >> .plot.gpl;
echo "" >> .plot.gpl;

chmod +x .plot.gpl;
sleep 0.1;
gnuplot -c .plot.gpl;

exit 0;

# gnuplot -persist <<-EOFMarker
#     set terminal png size 1280, 800
#     set output $output

#     set title sprintf("%s 1 byte contigously\nHigher is better", "tll")
#     set ylabel "Number of (millions) operations per seconds"
#     set xlabel sprintf("Number of threads (Max CPU: %s)", "4")

#     arg3 = 4
#     f(x) = (x == 1) ? 0. : (x == arg3/2) ? 1. : x/arg3 + 1
#     g(x) = (x == 0) ? 1. : (x == 1) ? (arg3/2) : (x - 1) * arg3

#     set nonlinear x via f(x) inv g(x)

#     # set x2tics
#     set y2tics

#     # set key reverse Left outside
#     set grid

#     set style line 1 lc rgb '#d32f2f' pt 6 ps 1 lt 1 lw 3
#     set style line 2 lc rgb '#ff6659' pt 6 ps 1 lt 1 lw 2
#     set style line 3 lc rgb '#512da8' pt 6 ps 1 lt 1 lw 2
#     set style line 4 lc rgb '#8559da' pt 6 ps 1 lt 1 lw 2
#     set style line 5 lc rgb '#0288d1' pt 1 ps 1 lt 1 lw 2
#     set style line 6 lc rgb '#5eb8ff' pt 6 ps 1 lt 1 lw 2
#     set style line 7 lc rgb '#689f38' pt 6 ps 1 lt 1 lw 2
#     set style line 8 lc rgb '#99d066' pt 6 ps 1 lt 1 lw 2
#     set style line 9 lc rgb '#f57c00' pt 6 ps 1 lt 1 lw 2
#     set style line 10 lc rgb '#ffad42' pt 6 ps 1 lt 1 lw 2

#     plot sprintf('%s', "bm_throughput.dat") u 1:2:xtic(1) t "boost" w lp ls 5, \
#           ""      u 1:4:xtic(1) t "moodycamel" w lp ls 7, \
#           ""      u 1:6:xtic(1) t "CCFIFO" w lp ls 1

# EOFMarker