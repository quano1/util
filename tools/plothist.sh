#!/bin/bash

mkdir -p tmp

for input in "$@"; do

    output=`echo $input | sed -e '/\//{ s/.*\/\([^.]*\)\.dat/\1/g; q; }' -e 's/\([^\.]*\)\.dat/\1/;'`;
    output=tmp/${output}.hist.`date +'%y%m%d_%H%M'`.png
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

    rm -f .plothist.gpl;
    > .plothist.gpl;

    echo "set terminal png size 1280, 800" >> .plothist.gpl;
    echo "set output \"$output\"" >> .plothist.gpl;
    echo "set title \"$title\"" >> .plothist.gpl;
    echo "set ylabel \"$yasix\"" >> .plothist.gpl;
    echo "set xlabel \"$xasix\"" >> .plothist.gpl;
    echo "set y2tics" >> .plothist.gpl;
    echo "set grid" >> .plothist.gpl;
    echo "set key right top" >> .plothist.gpl;
    echo "set key reverse Left outside" >> .plothist.gpl;

    echo "set style data histogram" >> .plothist.gpl;
    echo "set style histogram cluster gap 1" >> .plothist.gpl;
    echo "set style fill solid" >> .plothist.gpl;
    echo "set boxwidth 0.75" >> .plothist.gpl;

    echo "num_cpus = $num_cpus" >> .plothist.gpl;
    echo "set yrange [0:]" >> .plothist.gpl;
    # echo "set logscale y" >> .plothist.gpl;

    echo "plot for [col_=2:${cols}] '$input' u col_:xtic(1) ti col" >> .plothist.gpl;

    chmod +x .plothist.gpl;
    sleep 0.1;
    gnuplot -c .plothist.gpl;
    sleep 0.1;
done

exit 0;
