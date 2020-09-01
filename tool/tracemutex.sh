#!/bin/bash

sed -e 's#^.\(.*\).$#\1#g' $1 | \
gawk -F "}{" '{
    if ( $1 == "T"  &&  $0 ~ /}{tm_/ )
    {
        tid = int($3);
        if (NF > 8)
        {
            file = $6;
            mtx = $9;
            # ctx = file""mtx;
            # ctx = "{"file"}{"mtx"}";
            ctx = "{"file"}("mtx")";
            crr_list[tid][length(crr_list[tid])] = ctx;
            if (length(crr_list[tid]) > 1)
            {
                # printf "%d %d ", NR, tid;
                for (i=0; i < (length(crr_list[tid])-1); i++) {
                    snode=crr_list[tid][i];
                    enode=ctx;
                    graphs[snode][enode][tid] = 1;
                }
                # printf "%s\n", ctx;
            }
            printf "%*d t%*d %.*s%s\n", 7, NR, 2, tid, (length(crr_list[tid])-1), "-----------------------------", ctx;
        }
        else
        {
            # mutex[tid]--;
            delete crr_list[tid][length(crr_list[tid]) - 1];
            # print "delete: "length(crr_list[tid]);
        }
    }
}
END {
    dl_list[0]; delete dl_list[0];
    printf "\nGraphs\n";
    for (snode in graphs) {
        printf "%s\n", snode
        for (enode in graphs[snode]) {
            printf "\t%s ", enode
            for (tid in graphs[snode][enode]) {
                printf "%d ", tid
                checked_list[snode][enode][tid]=1;
                if (enode in graphs && snode in graphs[enode]) {
                    for (ctid in graphs[enode][snode]) {
                        if (checked_list[enode][snode][ctid] == 1) continue;
                        dl_list[length(dl_list)] = snode"-"enode" : "tid"-"ctid;
                    }
                }
            }
            print "";
        }
    }

    printf "\nDead lock context: " length(dl_list) "\n";

    for (i=0; i< length(dl_list); i++) {
        print dl_list[i];
    }
}'

#  | \
# gawk '{

# }'

