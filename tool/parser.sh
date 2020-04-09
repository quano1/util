#!/bin/bash

# head -n 9 test_lmngr.log | \

dd if=../tests/fd_t.log iflag=skip_bytes,count_bytes,fullblock bs=$((4096)) skip=$((0)) count=$((4096*1024)) 2> /tmp/null | \
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
			printf "{%s}{%s}%*s{%s}{+%s}{%s %s %s}\n", time_, thread_, obj_lst_[thread_]+1, " ", type_, msg_, line_, func_, file_;
			obj_lst_[thread_]=obj_lst_[thread_]+4;
		}
		else
		{
			printf "{%s}{%s}%*s{%s %s %s}{%s}{%s}\n", time_, thread_, obj_lst_[thread_]+1, " ", line_, func_, file_, type_, msg_;
		}
	}
	else
	{
		obj_lst_[thread_]=obj_lst_[thread_]-4;
		printf "{%s}{%s}%*s{%s}{-%s}\n", time_, thread_, obj_lst_[thread_]+1, " ", type_, $4;
	}

}
END {
	# for (key in obj_lst)
	# 	print key "\t\t" obj_lst[key];
}
'