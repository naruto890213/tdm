#!/bin/sh

TDM_SERVER_DIR=/home/SubServer/TerminalDeviceManage

PS="ps aux"

tdmServer_ret=" "
tdmServer_ret=`${PS} | grep tdmServerD | grep -v grep`
if [ -z "${tdmServer_ret}" ]
then
{
	if [ -x ${TDM_SERVER_DIR}/tdmServerD ]
  	then
	{
		${TDM_SERVER_DIR}/tdmServerD & 
	}
	fi
}
fi

echo "start tdm service ..."

#end

