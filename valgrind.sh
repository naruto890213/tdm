#!/bin/sh
	echo "zrway.com" |sudo valgrind --tool=memcheck --leak-check=full bin/tdm
