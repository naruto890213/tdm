function GetPID #PsUser #PsName
{
    PsUser=$1
    PsName=$2
    pid=`ps aux|grep $PsName|grep $PsUser|grep Sl|awk '{print $2}'`
    echo $pid
}

function GetCpu
{
    CpuValue=`ps -p $1 -o pcpu |grep -v CPU | awk '{print $1}' | awk -F. '{print $1}'`
    echo $CpuValue
}

function CheckCpu
{
    cpu=`GetCpu $1`

    echo "The $2 CPU is $cpu" >> shell.txt

    if [ "-$cpu" == "-" ]
    then
    {
        echo "The process is not found"
        return 1;
    }
    fi

    if [ $cpu -gt 50 ]
    then
    {
        echo "The usage of cpu is larger than 80%" >> shell.txt
		echo "zrway.com" |sudo kill $1 
    }   
    fi
}

function GetMem
{
    MEMUsage=`ps -o vsz -p $1|grep -v VSZ`
    (( MEMUsage /= 1000))
    echo $MEMUsage
}

function GetDes
{
    DES=`ls /proc/$1/fd | wc -l`
    echo $DES
}

function GetSysCPU
{
   CpuIdle=`vmstat 1 5 |sed -n '3,$p' |awk '{x = x + $15} END {print x/5}' |awk -F. '{print $1}'`
   CpuNum=`echo "100-$CpuIdle" | bc`
   echo $CpuNum
}

function Listening
{
    TCPListeningnum=`netstat -an | grep ":$1 " | awk '$1 == "tcp" && $NF == "LISTEN" {print $0}' | wc -l`
    UDPListeningnum=`netstat -an|grep ":$1 " |awk '$1 == "udp" && $NF == "0.0.0.0:*" {print $0}' | wc -l`
    (( Listeningnum = TCPListeningnum + UDPListeningnum ))
    if [ $Listeningnum == 0 ]
    then
    {
        echo "0"
    }
    else
    {
       echo "1"
    }
    fi
}

function GetDiskSpc
{
    if [ $# -ne 1 ]
    then
        return 1
    fi

    Folder="$1$"
    DiskSpace=`df -k |grep $Folder |awk '{print $5}' |awk -F% '{print $1}'`
    echo $DiskSpace
}

function Chech_Tcp_Server_Port_Stat
{
    count_result=0
    stat_num=`netstat -an|egrep $1 |awk '$6 == "LISTEN" && $1 == "tcp" {print $0}' |wc -l`
    if [ $stat_num -ge 1 ]
    then
    {
        count_result=0
        echo "$1 port is normal!"
    }
    else
    {
        echo "$1 port is abnormal, please check"
    }
    fi
    return 0
}
