echo "kill the bypass daemon"
bb=(`ps -ef|grep ./ctl|awk '{print $2}'`)
kill -9 ${bb[0]} > /dev/null 2>&1
#kill -9 ${bb[1]}> /dev/null 2>&1