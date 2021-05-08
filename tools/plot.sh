#!/bin/bash

mkdir -p tmp

for input in "$@"; do

    output=`echo $input | sed -e '/\//{ s/.*\/\([^.]*\)\.dat/\1/g; q; }' -e 's/\([^\.]*\)\.dat/\1/;'`;
    output=tmp/${output}.`date +'%y%m%d_%H%M'`.png
    echo ${output}

    unset texts
    declare -a texts

    while IFS= read -r line
    do
        line2=`echo "$line" | sed -n -e '/^\#/{ s/^.//; p; }'`;
        [[ ! -z $line2 ]] && {
            texts+=("${line2}");
        }
    done < $input

    # num_cols=${#texts[@]};
    # echo "Parsed: $((num_cols-4))";
    # echo "Parsed: ${#texts[@]}";
    [[ ${#texts[@]} -lt 4 ]] && { echo "something wrong"; exit 1; };

    cols=${texts[0]}
    cols=$((cols+1));
    num_cpus=${texts[1]}
    title=${texts[2]}
    yasix=${texts[3]}
    xasix=${texts[4]}

    [[ -z ${num_cpus} ]] && { echo "cpus == 0"; exit 1; };

    rm -f .plot.gpl;
    > .plot.gpl;

    echo "set terminal png size 1280, 800" >> .plot.gpl;
    echo "set output \"$output\"" >> .plot.gpl;
    echo "set title \"$title\"" >> .plot.gpl;
    echo "set ylabel \"$yasix\"" >> .plot.gpl;
    echo "set xlabel \"$xasix\"" >> .plot.gpl;
    echo "set y2tics" >> .plot.gpl;
    echo "set grid" >> .plot.gpl;
    # echo "set key left top" >> .plot.gpl;
    echo "set key reverse Left outside" >> .plot.gpl;

    echo "num_cpus = $num_cpus" >> .plot.gpl;
    # echo "set xrange [0:]" >> .plot.gpl;
    echo "set yrange [0:]" >> .plot.gpl;

    echo "plot for [col_=2:${cols}] '$input' u col_:xtic(1) w lp pt 6 lw 2 t columnheader " >> .plot.gpl;

    chmod +x .plot.gpl;
    sleep 0.1;
    gnuplot -c .plot.gpl;

done

exit 0;
