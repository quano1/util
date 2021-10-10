#!/bin/bash
#MIT License
#Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)


# head -n 9 test_lmngr.log | \
# while [[ eof_ -eq 0 ]]; do
# sort -t "}" -k2 | \

COLOR_ENABLED=${2:-0};
# sort_enabled=${3:-0};

function parse()
{
# apt-cache show "$package" | awk '/^Package: /{p=$2} /^Version: /{v=$2} /^Architecture: /{a=$2} /^$/{print "apt-get download "p"="v" -a="a}'
    gawk -F  "}{" '
    BEGIN {
        COLOR_ENABLED_='"$COLOR_ENABLED"'
        DIFF_='"$1"'

        if (COLOR_ENABLED_ == 1) {
            RESET="\033[0m"
            split("\033[91m;\033[92m;\033[93m;\033[94m;\033[95m;\033[96m;\033[97m;\033[31m;\033[32m;\033[33m;\033[34m;\033[35m;\033[36m;\033[37m;", colorlist, ";")
        }

        total_size = 0;
    }
    {
        type_=$1
        time_=$2+DIFF_
        thread_=int($3)
        lvl_=$4
        curr_ctx_=$5
        int_time_=int(time_);
        time_=int((time_ - int_time_) * int(1e9));
        date_=strftime("%y-%b-%d %T",int_time_);

        if(NF == 8) {

            if (COLOR_ENABLED_ == 1) {

                if (thread_color_[thread_] == "") {
                    index_ = (thread_%length(colorlist)) + 1;
                    thread_color_[thread_] = colorlist[index_];
                }

                switch (type_) {
                    case "T":
                        type_color_ = "\033[1;44m";
                        type_text_= RESET;
                        break;
                    case "I":
                        type_color_ = "\033[1;42m";
                        type_text_= RESET;
                        break;
                    case "W":
                        type_color_ = "\033[1;43m"
                        type_text_= "\033[1m";
                        break;
                    case "F":
                        type_color_ = "\033[1;41m"
                        type_text_= "\033[7m";
                        break;
                    default:
                        type_color_ = RESET;
                        type_text_= RESET;
                        break;
                }
            }

            func_line_=sprintf("%s %s", $6, $7);;
            # line_=$7;
            msg_=$8;
            trace_diff_=""
            if(type_ == "T") {

                if(match(msg_, "^+")) {
                    traces_[thread_][lvl_]=$2
                } else {
                    trace_diff_=sprintf("%9d (ns)", ($2 - traces_[thread_][lvl_])*1e9);
                    if(COLOR_ENABLED_ == 1) {
                        type_color_ = "\033[46m";
                    }
                }

            } else {

                # if(lvl_ > 0) {
                #     trace_diff_=sprintf("%9d (ns)", ($2 - traces_[thread_][lvl_-1])*1e9);
                # } else {
                #     trace_diff_=""
                # }

            }

            date_time_=sprintf("%s.%d", date_, time_);
            shift_=sprintf("%.*s", (lvl_), "                               ");
            # msg_=sprintf("%s%s%s", type_text_, msg_, RESET);
            printf "%s" \
                "%s {%20.20s} %20.20s %-20.20s%s " \
                "%s{%s}%s" \
                "%s"\
                "%s{%s}%s "\
                "%s\n",
                date_time_, 
                thread_color_[thread_], thread_, curr_ctx_, func_line_, RESET,
                type_color_, type_, RESET,
                shift_,
                thread_color_[thread_], msg_, RESET,
                trace_diff_;

        } else {
            print "ERROR: "$0;
        }
    }
    END {}';

}

# block_=1024;
# skip_=0;
# cnt_=1024;

main() {

# total_size=`ls -l $1 | awk '{print $5}'`;
DIFF_=`head -n1 $1 | awk '{print $2 - $1;}'`
# echo $DIFF_;

tail -n +2 $1 | sort -t "}" -k 2,2 -s | sed -e 's#{\(.*\)}#\1#' | parse $DIFF_
}

main $*
