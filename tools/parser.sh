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
        int_time_=int(time_);
        time_=int((time_ - int_time_) * int(1e9));
        thread_=int($3)
        lvl_=$4
        curr_ctx=$5

        # print strftime("%d-%m-%y %H-%M-%S",int_time_);
        # date_=strftime("%F %T",int_time_);

        date_=strftime("%y-%b-%d %T",int_time_);
        
        if (COLOR_ENABLED_ == 1) {
            if (thread_color_[thread_] == "") {
                index_ = (thread_%length(colorlist)) + 1;
                thread_color_[thread_] = colorlist[index_];
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

        if(NF == 8) {
            func_=$6;
            line_=$7;
            msg_=$8;
            if(COLOR_ENABLED_ == 1) {
                if(type_ == "{T") {
                    if(match(msg_, "^-")) { 
                        type_color_ = "\033[46m";
                    }
                }
            }
            printf "{%s.%d}%s%s}%s%s{%s}%2d%.*s{%s}{%s %s}%s %s{%s%s\n", 
                date_, time_, type_color_, type_, RESET, thread_color_[thread_], thread_, lvl_, (lvl_), "-----------------------------", 
                curr_ctx, func_, line_, RESET, 
                type_text_, msg_, RESET;
        }
        else {
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

tail -n +2 $1 | sort -t "}" -k 2,2 -s | parse $DIFF_
}

main $*
