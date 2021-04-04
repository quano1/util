#!/bin/bash
#MIT License
#Copyright (c) 2021 Thanh Long Le (longlt00502@gmail.com)

sed -i -e '
/[^_a-zA-Z0-9]\(#\|namespace\|for_each\|while\|for\|if\|do\|switch\) *(/!{
    /\/\{2,\}[ ]*\([_a-zA-Z0-9]\+[<>: ]*\) *[~_a-zA-Z0-9]\+(/{b;}
    /\([_a-zA-Z0-9]\+[<>: ]*\) *[~_a-zA-Z0-9]\+(.*)[,;]/{b;}
    /\([_a-zA-Z0-9]\+[<>: ]*\) *[~_a-zA-Z0-9]\+(/{
        :cont
        /{/ {
            /[^:]:[^:]/ {
                /[^_a-zA-Z0-9] *{/bdo;
                n;bcont
            }
            # /] *(/{n;bcont}
            /{ *$/bdo;
            /[^_a-zA-Z0-9]const *{/bdo;
            /[_a-zA-Z0-9(=] *{/ {n;bcont}
            :do
            s/\(.*\){/\1{TLL_GLOGTF();/
            b;
        }
        /;/b
        n;bcont
    }
}
' $1

if [[ ! -z "$2" ]]; then
    sed -i -e ':a;N;$!ba;
        s#\(\#include.*\)*\(\#include[^\n]*\)#\1\2\n\'"$2"'\n#
    ' $1
fi
