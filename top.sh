#!/bin/sh
	top -H -d 1 -p `ps aux | grep tdm | grep root | grep -v tdmServerD | awk '{print $2}'`
