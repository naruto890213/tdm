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
DIR_INC =  ./inc
DIR_SRC = ./src
DIR_OBJ = ./obj
DIR_BIN = ./
#DIR_LIB_PUB     = ../public 
SRC = $(wildcard ${DIR_SRC}/*.c)  
OBJ = $(patsubst %.c,${DIR_OBJ}/%.o,$(notdir ${SRC})) 
 
TARGET = tdm
 
BIN_TARGET = ${DIR_BIN}/${TARGET}
 
CC = gcc
CFLAGS = -g -Wall -I${DIR_INC_CUR} -I${DIR_INC}
 

${DIR_OBJ}/%.o:${DIR_SRC}/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ 

${BIN_TARGET}:${OBJ}
	$(CC) $(OBJ) -o $@ -lpthread -levent
	
.PHONY:clean
clean:
	find $(DIR_OBJ) -name "*.o" -a -type f -exec rm  {} \;
all:$(BIN_TARGET)
