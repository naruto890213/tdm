#!/bin/sh
MYDATE=`date +%y%m%d%H%M`
SPM_RUN_DIR=/home/SubServer/src/src/TerminalDeviceManage
LOG_DIR=$SPM_RUN_DIR/logbak_$MYDATE
echo "back up log files!!!"
mkdir $LOG_DIR
mv $SPM_RUN_DIR/logs/log.txt $LOG_DIR/
mv $SPM_RUN_DIR/TDM_err.log $LOG_DIR/
mv $SPM_RUN_DIR/TDM.log $LOG_DIR/

echo "kill tdm !!!"
PS_RET=""
PS="ps aux"
SPM_Pid1=`ps -aux | grep tdm |grep -v grep |grep -v tdmServerD |awk '{print$2}'`
echo "the spm's pid is $SPM_Pid1"
kill -9 $SPM_Pid1
