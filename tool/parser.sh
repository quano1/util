#!/bin/bash

dd if=test_lmngr.log iflag=skip_bytes,count_bytes,fullblock bs=4096 skip=0 count=4096 2> /tmp/null | \
sort -t \; -k1 | \
sed 's#/home/helloworld/workspace/util/tests/##g' | \
awk -F  ";" '
BEGIN{
}
{
	if (NF==10)
	{
		time_=$1; pid_=$2; tid_=$3; obj_addr_=$4;
		log_type_=$5; indent_=$6;
		file_=$7;
		func_=$8;
		line_=$9;
		log_content_=$10;
		log_msg=func_":"line_":"log_content_

		if(obj_addr_ == "0x0")
			key = "::"file_;
		else
			key = file_":"obj_addr_;

		if (line_ != -1)
		{

			if(obj_lst[key] == "")
			{
				obj_lst[key] = log_content_;
			}

			if(ctx_current[tid_] == "")
			{
				ctx_current[tid_] = key;
				ctx_name[tid_] = log_content_;
				print time_":" ctx_name[tid_] "\t" ctx_current[tid_] "\tSTART ";
			}
			else if(ctx_current[tid_] == key)
			{
				print time_":" ctx_name[tid_] "\t" key "\tself call\t" log_msg;
			}
			else
			{
				print time_":" ctx_name[tid_] "\t" ctx_current[tid_] "\t===>\t" key "\t" log_msg;
				ctx_prev[tid_] = ctx_current[tid_];
				ctx_current[tid_] = key;
			}
		}
		else
		{
			print time_":" ctx_name[tid_] "\t" ctx_prev[tid_] "\t<---\t" ctx_current[tid_];
			ctx_current[tid_] = ctx_prev[tid_];
		}
	}
}
END {
	# for (key in obj_lst)
		# print key "\t\t" obj_lst[key];

}
'