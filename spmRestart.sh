#!/bin/sh
MYDATE=`date +%y%m%d%H%M`
SPM_RUN_DIR=/home/spm/run
LOG_DIR=$SPM_RUN_DIR/logbak_$MYDATE
echo "back up log files!!!"
mkdir $LOG_DIR
mv $SPM_RUN_DIR/logs/log.txt $LOG_DIR/log.txt
mv $SPM_RUN_DIR/spm.log $LOG_DIR/spm.log
mv $SPM_RUN_DIR/spm_err.log $LOG_DIR/spm_err.log

echo "kill spm !!!"
PS_RET=""
PS="ps aux"
SPM_Pid1=`ps -aux |grep spm |grep -v grep |grep -v spmServerD |awk '{print$2}'`
echo "the spm's pid is $SPM_Pid1"
kill -9 $SPM_Pid1
#sleep 2 
#SPM_Pid2=`ps -aux |grep spm |grep -v grep |grep -v spmServerD |awk '{print$2}'`
#echo "restarted spm's pid is $SPM_Pid2"
