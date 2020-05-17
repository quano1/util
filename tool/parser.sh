#!/bin/bash

# head -n 9 test_lmngr.log | \

eof_=0

while [[ eof_ -eq 0 ]]; do

dd if=$1 iflag=skip_bytes,count_bytes,nonblock bs=4K skip=32K count=32K 2> /tmp/null | \
sort -t "}" -k2 | \
sed -e 's#^.\(.*\).$#\1#g' | \
awk -F  "}{" '
BEGIN {
    T_RED="\033[0;31m"
    T_GREEN="\033[0;32m"
    T_YELLOW="\033[0;33m"
    T_BLUE="\033[0;34m"
    RESET="\033[0m"
    B_RED="\033[0;41m"
    B_GREEN="\033[0;42m"
    B_YELLOW="\033[0;43m"
    B_BLUE="\033[0;44m"

    command = ("ls -a /tmp/.t_*")
    while ((command | getline thread_) > 0) {
        thread_=gensub(/^.*.t_(.*)/, "\\1", "g", thread_);

        subcommand = ("cat /tmp/.t_"thread_)
        if ((subcommand | getline lvl_) > 0) {
            stack_lvl_[thread_] = lvl_*2;
        }
        close(subcommand)
        printf "%s: %d\n", thread_, stack_lvl_[thread_];
    }
    close(command)
}
{
    type_=$1
    time_=$2
    thread_=$3

    if(NF > 5) {
        file_=$4;
        func_=$5;
        line_=$6;
        msg_=$7;
        if (type_ == "T") {
            printf "%s{%s}{%s}{%s}%2d%*s{%s %s %s}{+%s}%s\n", T_BLUE, time_, thread_, type_, stack_lvl_[thread_]/2, stack_lvl_[thread_]+1, " ", file_, func_, line_, msg_, RESET;

            stack_lvl_[thread_]=stack_lvl_[thread_]+2;
        }
        else {
            switch (type_) {
                case "W":
                    start_color = T_YELLOW;
                    break;
                case "F":
                    start_color = T_RED;
                    break;
                default:
                    start_color = RESET;
                    break;
            }

            printf "%s{%s}{%s}{%s}%2d%*s{%s %s %s}{%s}%s\n", start_color, time_, thread_, type_, stack_lvl_[thread_]/2, stack_lvl_[thread_]+1, " ", file_, func_, line_, msg_, RESET;
        }
    }
    else if (NF == 5) {
        stack_lvl_[thread_]=stack_lvl_[thread_]-2;
        printf "%s{%s}{%s}{%s}%2d%*s{-%s}{%s}%s\n", T_YELLOW, time_, thread_, type_, stack_lvl_[thread_]/2, stack_lvl_[thread_]+1, " ", $4, $5, RESET;
    }
    # else {
    #   print "ERROR: " $0
    # }

}
END {
    for (key in stack_lvl_) {
        printf "%d\n", stack_lvl_[key]/2 > "/tmp/.t_"key;
        close("/tmp/.t_"key)
    }
}
' | less -r

eof_=1
done