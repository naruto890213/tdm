#!/bin/bash
IP=`ifconfig -a | grep -v inet6 | grep inet | grep -v 127|awk '{print $2}' | awk -F : '{print $2}'`
echo "$IP"
