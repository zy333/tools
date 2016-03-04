#!/bin/bash
#该脚本为tcp server的监控脚本，配置在cron中每60s执行一次
#使用前先定义下面变量
#cron名称
script_name="swoole_tcp_server.php"
#脚本路径
cron_dir=`pwd`
cron_path=${cron_dir}"/"${script_name}
#tcp server的进程数量 -- script_name 中的 worker_num + 1
proc_num=4

#检查进程数量
count=`ps -fe |grep $script_name | grep -v "grep"  | wc -l`
echo $count
#进程数量不符则重启
if [ $count -lt $proc_num ]; then
ps -eaf |grep $script_name | grep -v "grep"| awk '{print $2}'|xargs kill -9
sleep 2
ulimit -c unlimited
php $cron_path
echo "restart";
fi