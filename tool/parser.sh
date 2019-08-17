#!/bin/bash

# head -n 9 test_lmngr.log | \

dd if=big.log iflag=skip_bytes,count_bytes,fullblock bs=$((4096)) skip=$((0)) count=$((4096*256)) 2> /tmp/null | \
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
		log_msg=func_" "line_" "log_content_

		if(obj_addr_ == "0x0")
			key = file_;
		else
			key = file_":"obj_addr_;

		if (line_ != -1)
		{
			if(obj_lst[key] == "")
			{
				obj_lst[key] = log_content_;
			}

			stack_obj[tid_][indent_] = key;
			if(ctx_current[tid_] == "")
			{
				# START
				ctx_current[tid_] = key;
				# print "Note over " key ": " obj_lst[key] " START";
				print time_ ";" pid_ ";" tid_ ";BEG;" ctx_current[tid_] ";" func_ ";" line_ ";" obj_lst[key] " BEGIN"
			}
			else if(ctx_current[tid_] == key)
			{
				# SELF CALL
				# print key "->" key ":" log_msg;
				# TODO for log_type_

				if(log_type_ == "TRAC")
					print time_ ";" pid_ ";" tid_ ";SEL;" ctx_current[tid_] "\t" ctx_current[tid_] ";" func_ ";" line_ ";" log_content_;
				else
					print time_ ";" pid_ ";" tid_ ";OTH-" log_type_ ";" ctx_current[tid_] "\t" ctx_current[tid_] ";" func_ ";" line_ ";" log_content_;
			}
			else
			{
				# DISPATCH
				# print ctx_current[tid_] "->" key ":" log_msg;
				print time_ ";" pid_ ";" tid_ ";DIS;" ctx_current[tid_] "\t" key ";" func_ ";" line_ ";"
				ctx_current[tid_] = key;
			}
		}
		else
		{
			# RETURN
			if(indent_ > 0)
			{
				# print ctx_current[tid_] "-->" stack_obj[tid_][indent_ - 1] ":" log_msg;
				print time_ ";" pid_ ";" tid_ ";RET;" ctx_current[tid_] "\t" stack_obj[tid_][indent_ - 1]
				ctx_current[tid_] = stack_obj[tid_][indent_ - 1];
			}
			else
			{
				# END
				# print "\t\t"ctx_current[tid_] "-->" ctx_current[tid_] ":" log_msg;
				# print "Note over " ctx_current[tid_] ": "obj_lst[ctx_current[tid_]]" END";
				print time_ ";" pid_ ";" tid_ ";END;" ctx_current[tid_] ";" obj_lst[ctx_current[tid_]] " END"
			}
		}
	}
}
END {
	# for (key in obj_lst)
	# 	print key "\t\t" obj_lst[key];
}
'