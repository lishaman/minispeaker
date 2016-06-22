timeout=1800
times=10
line=0
md5_wrong=0

update_path=$1
update_version=$2
update_num=$3


while [ $timeout -ne 0 ]
do
	echo update path is $update_path
	echo update version is $update_version
	echo update num is $update_num
	echo "wget -c $update_path/update_$update_version/info_$update_version"
	wget -c $update_path/update_$update_version/info_$update_version -P /tmp
	if [ $? -eq 0 ]
	then
		chmod 777 /tmp/info_$update_version

		echo "wget -c $update_path/update_$update_version/update$update_num.zip -P /tmp"
		wget -c $update_path/update_$update_version/update$update_num.zip -P /tmp

		if [ $? -eq 0 ]
		then


while [ $times -ne 0 ]
do
			echo ==================begin md5=================================
			let line=$update_num+1
			cat /tmp/info_$update_version | awk '{if(NR==v1)print $0;}' v1=$line
			cat /tmp/info_$update_version | awk '{if(NR==v1)print $0;}' > /tmp/md5_temp v1=$line
			cd /tmp
			md5sum -c md5_temp

			if [ $? -eq 0 ]
			then

				cd /tmp
				unzip update$update_num.zip
				cd updatezip$update_num/script
				. update.script
				rm -rf /tmp/update*
				rm  /tmp/info_$update_version
				break
			else
				echo md5 wrong times $md5_wrong > /tmp/md5file
				rm -rf /tmp/update*
				wget -c $update_path/update_$update_version/update$update_num.zip -P /tmp
				let times=$times-1
				md5_wrong=$(($md5_wrong+1))
			fi
		done
break
		else
			echo "wget fail; wait 2s;reconnect"
			sleep 2
			let timeout=$timeout-1
		fi

	else
		echo "wget fail; wait 2s;reconnect"
		sleep 2
		let timeout=$timeout-1
	fi

done

if [ $timeout -eq 0 ]
then
	poweroff
fi
