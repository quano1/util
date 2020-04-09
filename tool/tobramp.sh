#!/bin/bash

# head -n 9 test_lmngr.log | \

# A->B: Normal line
# B-->C: Dashed line
# C->>D: Open arrow
# D-->>A: Dashed open arrow
# Note over A: Note over A

dd if=../tests/fd_t.log iflag=skip_bytes,count_bytes,fullblock bs=$((4096)) skip=$((0)) count=$((4096*1024)) 2> /tmp/null | \
sort -t "}" -k2 | \
sed -e 's#^.\(.*\).$#\1#g' | \
awk -F  "}{" '
BEGIN{}
{
	type_=$1
	time_=$2
	thread_=$3

	invoke_="->"
	return_="-->"
	self_call_="Note over "

	if(NF > 5)
	{
		file_=$4;
		func_=$5;
		line_=$6;
		msg_=$7;

		if (type_ == "T")
		{
			if (curr_[thread_]=="")
			{
				curr_[thread_]=file_;
			}
			
			if (curr_[thread_] == file_)
			{
				# self call
				printf "Note over %s: +%s\n", file_, msg_;
			}
			else
			{
				# invoke
				prev_[thread_] = curr_[thread_];
				printf "%s->%s: %s\n", curr_[thread_], file_, msg_;
				curr_[thread_] = file_;
			}
		}
		else
		{
			# D I F
			printf "Note right of %s: %s\n", file_, msg_;
		}
	}
	else
	{
		msg_=$4
		# return
		if (curr_[thread_] == file_)
		{
			# self call return
			printf "Note over %s: -%s\n", file_, msg_;
		}
		else
		{
			printf "%s-->%s: %s\n", curr_[thread_], prev_[thread_], msg_;
			curr_[thread_] = prev_[thread_];
		}
	}

}
END {
	# for (key in obj_lst)
	# 	print key "\t\t" obj_lst[key];
}
'