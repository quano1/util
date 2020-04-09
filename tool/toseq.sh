#!/bin/bash

# head -n 9 test_lmngr.log | \

# A->B: Normal line
# B-->C: Dashed line
# C->>D: Open arrow
# D-->>A: Dashed open arrow
# note over A: note over A
#https://sequencediagram.org/

dd if=$1 iflag=skip_bytes,count_bytes,fullblock bs=$((4096)) skip=$((0)) count=$((4096*1024)) 2> /tmp/null | \
sed -e 's#^.\(.*\).$#\1#g' | \
awk -F  "}{" '
BEGIN{}
{
	type_=$1
	time_=$2
	thread_=$3

	curr_lvl_ = stack_call_[thread_][0];
	# printf "Thread: %s, LEVEL: %s\n", thread_, curr_lvl_;

	if(NF > 5)
	{
		file_=$4;
		func_=$5;
		line_=$6;
		msg_=$7;

		if (type_ == "T")
		{
			next_lvl_ = curr_lvl_ + 1;
			stack_call_[thread_][0] = next_lvl_;
			stack_call_[thread_][next_lvl_]=file_;

			if (curr_lvl_ == 0 || stack_call_[thread_][curr_lvl_] == file_)
			{
				# self call
				printf "note over %s: +%s\n", file_, msg_;
			}
			else
			{
				# invoke
				printf "%s->%s: %s\n", stack_call_[thread_][curr_lvl_], stack_call_[thread_][next_lvl_], msg_;
			}
		}
		else
		{
			# D I F
			printf "note right of %s: %s\n", stack_call_[thread_][curr_lvl_], msg_;
		}
	}
	else
	{
		msg_=$4
		next_lvl_ = curr_lvl_ - 1;
		stack_call_[thread_][0] = next_lvl_;

		# return
		if (next_lvl_ == 0 || stack_call_[thread_][next_lvl_] == stack_call_[thread_][curr_lvl_])
		{
			# self call return
			printf "note over %s: -%s\n", stack_call_[thread_][curr_lvl_], msg_;
		}
		else
		{
			printf "%s-->%s: ~%s\n", stack_call_[thread_][curr_lvl_], stack_call_[thread_][next_lvl_], msg_;
		}
	}

}
END {
	# for (key in obj_lst)
	# 	print key "\t\t" obj_lst[key];
}
'