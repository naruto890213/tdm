#CC = gcc
#CFLAGS = -O2
#TARGET = spm
#export OBJSDIR = ${shell pwd}/.objs
#$(TARGET):$(OBJSDIR) main.o
#	$(MAKE) -C sim_pool_manage
#	$(MAKE) -C public
#	$(CC) -o $(TARGET) $(OBJSDIR)/*.o
#main.o:%.o:%.c
#	$(CC) -c $< -o $(OBJSDIR)/$@ $(CFLAGS) -Iinc
#$(OBJSDIR):
#	mkdir -p $(OBJSDIR)
#clean:
#	-$(RM) $(TARGET)
#	-$(RM) $(OBJSDIR)/*.o
DIR_INC_CUR = ./
DIR_INC =  inc
DIR_SRC = src
DIR_OBJ = obj
DIR_BIN = bin
DIR_LIBEVNENT = libevent
SRC = $(wildcard ${DIR_SRC}/*.c)  
OBJ = $(patsubst %.c,${DIR_OBJ}/%.o,$(notdir ${SRC})) 
 
TARGET = tdm
 
BIN_TARGET = ${DIR_BIN}/${TARGET}
 
CC = gcc
CFLAGS = -g -Wall -I${DIR_INC_CUR} -I${DIR_INC}

.PHONY: all
all: prepare_libevent prepare_dir build
	echo "Compile tdm"

prepare_libevent:
	echo "Compile libevent"
	if [ ! -f /usr/local/lib/libevent.a ]; then	\
		echo "libevent no exist";	\
		if [ ! -f $(DIR_LIBEVNENT)/Makefile ]; then	\
			echo "libevent Makefile not exist!";	\
			cd $(DIR_LIBEVNENT) && ./configure;	\
		fi	\
		echo "libevent install"	\
		@make -C $(DIR_LIBEVNENT) install;	\
	fi

prepare_dir:
	echo "prepare_dir"
	if [ ! -d $(DIR_OBJ) ]; then \
		echo "$(DIR_OBJ) not exit"; \
		mkdir -p $(DIR_OBJ); \
	fi
	if [ ! -d $(DIR_BIN) ]; then \
		echo "$(DIR_BIN) not exit"; \
		mkdir -p $(DIR_BIN); \
	fi

build:$(BIN_TARGET)

clean:
	find $(DIR_OBJ) -name "*.o" -a -type f -exec rm  {} \;

${DIR_OBJ}/%.o:${DIR_SRC}/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ 

${BIN_TARGET}:${OBJ}
	$(CC) $(OBJ) -o $@ -lpthread -levent
