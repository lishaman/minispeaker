#!/bin/bash

[ ! -f $1 ] && echo "Not Found"

file_size=`ls -l $1 | cut -d' ' -f5`
if [ $file_size -lt 1024 ]; then # [0, 1KB)
	printf "%.2fB" "$file_size"
else if [ $file_size -lt $((1024 * 1024)) ]; then # [1KB, 1MB)
	printf "%.2fKB" `echo "scale=4;$file_size/1024"|bc`
else if [ $file_size -lt $((1024 * 1024 * 1024)) ]; then # [1MB, 1GB)
	printf "%.2fMB" `echo "scale=4;$file_size/1024/1024"|bc`
else if [ $file_size -lt $((1024 * 1024 * 1024 * 1024)) ]; then # [1GB, 1TB)
	printf "%.2fGB" `echo "scale=4;$file_size/1024/1024/1024"|bc`
else if [ $file_size -lt $((1024 * 1024 * 1024 * 1024 * 1024)) ]; then # [1TB, 1PB)
	printf "%.2fTB" `echo "scale=4;$file_size/1024/1024/1024/1024"|bc`
else if [ $file_size -lt $((1024 *1024 * 1024 * 1024 * 1024 * 1024)) ]; then # [1PB, 1EB)
	printf "%.2fPB" `echo "scale=4;$file_size/1024/1024/1024/1024/1024"|bc`
else # >= 1EB, such as EB, ZB, YB, DB, NB
	echo "`du -hs $1 | cut -f1`"
fi
fi
fi
fi
fi
fi
