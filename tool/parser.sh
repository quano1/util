#!/bin/bash

# head -n 9 test_lmngr.log | \

dd if=$1 iflag=skip_bytes,count_bytes,fullblock bs=$((4096)) skip=$((0)) count=$((4096*1024)) 2> /tmp/null | \
sort -t "}" -k2 | \
sed -e 's#^.\(.*\).$#\1#g' | \
awk -F  "}{" '
BEGIN{}
{
	type_=$1
	time_=$2
	thread_=$3

	if(NF > 5)
	{
		file_=$4;
		func_=$5;
		line_=$6;
		msg_=$7;
		if (type_ == "T")
		{
			printf "{%s}{%s}{%s}%2d%*s{%s %s %s}{+%s}\n", time_, thread_, type_, stack_lvl_[thread_]/2, stack_lvl_[thread_]+1, " ", line_, func_, file_, msg_;
			stack_lvl_[thread_]=stack_lvl_[thread_]+2;
		}
		else
		{
			printf "{%s}{%s}{%s}%2d%*s{%s %s %s}{%s}\n", time_, thread_, type_, stack_lvl_[thread_]/2, stack_lvl_[thread_]+1, " ", line_, func_, file_, msg_;
		}
	}
	else if (NF == 5)
	{
		stack_lvl_[thread_]=stack_lvl_[thread_]-2;
		printf "{%s}{%s}{%s}%2d%*s{-%s}{%s}\n", time_, thread_, type_, stack_lvl_[thread_]/2, stack_lvl_[thread_]+1, " ", $4, $5;
	}
	else
	{
		print "ERROR"
	}

}
END {
	# for (key in obj_lst)
	# 	print key "\t\t" obj_lst[key];
}
'