#!/bin/bash
#MIT License
#Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)


# head -n 9 test_lmngr.log | \
# while [[ eof_ -eq 0 ]]; do
# sort -t "}" -k2 | \

color_enabled=${4:-0};
sort_enabled=${5:-0};

function parse()
{
    # total_lines=`dd if=${1} iflag=skip_bytes,count_bytes,nonblock bs=${2} skip=${3} count=${4} | wc -l $1`;
    # sed -e 's#^.\(.*\).$#\1#g' | \
    # echo $total_lines;

    # dd if=${1} iflag=skip_bytes,count_bytes,nonblock bs=${2} skip=${3} count=${4} 2> /tmp/null | \
    # ( ((sort_enabled)) && (sort -t "}" -nsk 2,2) ) | \

    gawk -F  "}{" '
    BEGIN {
        # print "BEGIN"
        color_enabled='"$color_enabled"'
        if (color_enabled == 1) {
            RESET="\033[0m"
            split("\033[91m;\033[92m;\033[93m;\033[94m;\033[95m;\033[96m;\033[97m;\033[31m;\033[32m;\033[33m;\033[34m;\033[35m;\033[36m;\033[37m;", colorlist, ";")
        }
        # tcolor = 1;
        total_size = 0;
    }
    {
        type_=$1
        time_=$2
        thread_=int($3)
        lvl_=$4-1
        # split($5, ctxs, ":");
        prev_ctx=$5
        curr_ctx=$6
        # if (lvl_ > 4) next;
        # split($5, obj_, ":")

        if (color_enabled == 1) {
            if (thread_color_[thread_] == "") {
                index_ = (thread_%length(colorlist)) + 1;
                # print "INDEX: " index_;
                thread_color_[thread_] = colorlist[index_];
                # tcolor+=1;
                # if(tcolor > length(colorlist)) {
                #     tcolor = 1;
                # }
            }

            switch (type_) {
                case "{T":
                    type_color_ = "\033[1;44m";
                    type_text_= RESET;
                    break;
                case "{I":
                    type_color_ = "\033[1;42m";
                    type_text_= RESET;
                    break;
                case "{W":
                    type_color_ = "\033[1;43m"
                    type_text_= "\033[1m";
                    break;
                case "{F":
                    type_color_ = "\033[1;41m"
                    type_text_= "\033[7m";
                    break;
                default:
                    type_color_ = RESET;
                    type_text_= RESET;
                    break;
            }
        }

        # print match($0, "}$")

        if(NF < 8 || match($0, "}$") == 0)
        {
            print "ERROR: "$0;
            # print total_size > ".run.log"
            # exit 1;
        } else {

            if(NF == 9) {
                func_=$7;
                line_=$8;
                msg_=$9;
                if(type_ != "{T") lvl_+=1;
                printf "{%20.9f}%s%s}%s%s{%s}%2d%.*s{%s}{%s %s}%s %s{%s%s\n", 
                    time_, type_color_, type_, RESET, thread_color_[thread_], thread_, lvl_, (lvl_), "-----------------------------", 
                    curr_ctx, func_, line_, RESET, 
                    type_text_, msg_, RESET;
            }
            else if (NF == 8) {
                if (color_enabled == 1) {
                    type_color_ = "\033[46m";
                }
                func_=$7
                msg_=$8;
                printf "{%20.9f}%s%s}%s%s{%s}%2d%.*s{%s} %s%s {%s\n", 
                        time_, type_color_, type_, RESET, thread_color_[thread_], thread_, lvl_, (lvl_), "-----------------------------", 
                        curr_ctx, RESET, func_, msg_;
                # print $0
                # print NF
            }
            # total_size += length+1;
        }
    }
    END {
        # for (key in stack_lvl_) {
        #     printf "%d\n", stack_lvl_[key]/2 > "/tmp/.t_"key;
        #     close("/tmp/.t_"key)
        # }
        # print total_size + NR * 3;
        # print total_size;
        # print "END"
    }';

}

# total_size=`ls -l $1 | awk '{print $5}'`;
block_=1024;
# skip_=0;
# cnt_=1024;

# cnt_=1048576;
(dd if=${1} iflag=skip_bytes,count_bytes,nonblock bs=1024 skip=${2} count=${3} 2> /tmp/null ) | sort -t "}" -k 2,2 -s | parse;

    # ( ((sort_enabled)) && ( sort -t "}" -nsk 2,2 | parse ) || parse)

    # ((sort_enabled)) && ( sort -t "}" -nsk 2,2 | parse )

# parse $1 $block_ $2 $3;
# exit 1;

# total_size=`expr $total_size - $block_`;
# echo $total_size;
# rm -f run.log;

# while [[ $skip_ -le $total_size ]]; do
# for i in {1..5}; do
#     # echo $skip_ " " $cnt_;
#     tmp_=$cnt_;
#     parse $1 $block_ $skip_ $cnt_ >> run.log;
#     # echo $?
#     # echo $?;
#     # ls -l run.log | awk '{print $5}';
#     # if [[ ! -z .run.log ]]; then
#     if [[ $? -eq 1 ]]; then
#         tmp_=`cat .run.log`;
#         rm -f .run.log;
#     fi
#     skip_=`expr $skip_ + $cnt_ - $cnt_ + $tmp_`;
#     echo $skip_;
#     # skip_=`ls -l run.log | awk '{print $5}'`;
#     # cnt_=`expr $skip_ + $block_`;
# done

# # echo $skip_ " " $cnt_;
# tmp_=$skip_;
# if [[ ! -z .run.log ]]; then
#     tmp_=`cat .run.log`;
#     rm -f .run.log;
# fi
# parse $1 $block_ $skip_ $cnt_ >> run.log
