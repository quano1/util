#!/bin/bash

# head -n 9 test_lmngr.log | \

eof_=0

# while [[ eof_ -eq 0 ]]; do

dd if=$1 iflag=skip_bytes,count_bytes,nonblock bs=4K skip=0 count=2M 2> /tmp/null | \
sort -t "}" -k2 | \
sed -e 's#^.\(.*\).$#\1#g' | \
awk -F  "}{" '
BEGIN {
    RESET="\033[0m"

    split("\033[91m;\033[92m;\033[93m;\033[94m;\033[95m;\033[96m;\033[97m;\033[31m;\033[32m;\033[33m;\033[34m;\033[35m;\033[36m;\033[37m;", colorlist, ";")
    tcolor = 1;
}
{
    type_=$1
    time_=$2
    thread_=$3
    lvl_=$4
    obj_=$5

    if (thread_color_[thread_] == "") {
        thread_color_[thread_] = colorlist[tcolor];
        tcolor+=1;
        if(tcolor > length(colorlist)) {
            tcolor = 1;
        }
    }

    switch (type_) {
        case "T":
            type_color_ = "\033[44m";
            break;
        case "I":
            type_color_ = "\033[42m";
            break;
        case "W":
            type_color_ = "\033[43m"
            break;
        case "F":
            type_color_ = "\033[41m"
            break;
        default:
            type_color_ = RESET;
            break;
    }

    if(NF > 7) {
        file_=$6;
        func_=$7;
        line_=$8;
        msg_=$9;
        # type_color_=sprintf("\033[38;5;%d;42m",FG[thread_]);
        if (type_ == "T") {
            printf "{%s}%s{%s}%s%s{%s}%2d%*s{%s %s %s}{%s}%s{+%s}\n", time_, type_color_, type_, RESET, thread_color_[thread_], thread_, lvl_, lvl_, " ", file_, func_, line_, obj_, RESET, msg_;

            # stack_lvl_[thread_]=lvl_;
        }
        else {
            printf "{%s}%s{%s}%s%s{%s}%2d%*s{%s %s %s}{%s}%s{%s}\n", time_, type_color_, type_, RESET, thread_color_[thread_], thread_, lvl_, lvl_, " ", file_, func_, line_, obj_, RESET, msg_;
        }
    }
    else if (NF == 7) {
        type_color_ = "\033[46m";
        printf "{%s}%s{%s}%s%s{%s}%2d%*s{%s}%s{-%s}{%s}\n", time_, type_color_, type_, RESET, thread_color_[thread_], thread_, lvl_, lvl_, " ", obj_, RESET, $6, $7;
    }
    # else {
    #   print "ERROR: " $0
    # }

}
END {
    # for (key in stack_lvl_) {
    #     printf "%d\n", stack_lvl_[key]/2 > "/tmp/.t_"key;
    #     close("/tmp/.t_"key)
    # }
}
'
eof_=1
# done