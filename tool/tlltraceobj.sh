#!/bin/bash

sed -e 's#^.\(.*\).$#\1#g' $1 | \
gawk -F "}{" '{
    if ( $1 == "D" && $7 ~ /(Std)|(^emplace)|(^push)|(^pop)|(^operator)|(^clear)|(^erase)|(^insert)|(begin$)|(end$)|(^at)|(^find)|(^remove)/ && $9 ~ /^[0-9]+/ )
    {
        tid_ = int($3);
        func_ = $7;
        obj_ = $9;
        if (func_ ~ /^Std/) {
            graphs[obj_][tid_] = 1;
        } else if (func_ ~ /^~Std/) {
            if (length(graphs[obj_]) > 1) {
                printf "%s ", obj_;
                for (ttid_ in graphs[obj_]) {
                    printf "%d ", ttid_;
                }
                printf("\n");
            }
            delete graphs[obj_];
        } else {
            graphs[obj_][tid_] = 1;
        }
    }
}
END {
    printf "\nGraphs\n";
    for (tobj_ in graphs) {
        if (length(graphs[tobj_]) > 1) {
            printf "%s ", tobj_;
            for (ttid_ in graphs[tobj_]) {
                printf "%d ", ttid_;
            }
            printf("\n");
        }
    }
}'

#  | \
# gawk '{

# }'

