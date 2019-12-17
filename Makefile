DIR_SRC=src
DIR_BIN=bin

SRCS=$(wildcard ${DIR_SRC}/*.c)
BINS=$(foreach s,${SRCS},$(subst ${DIR_SRC}/,${DIR_BIN}/,${s:%.c=%}))

DEPS=utils.h

FLAGS=-g -Wall -Werror -std=gnu11 -lpthread -lm -lX11 -ldl -lGL
INCLUDES=-I.

CC=gcc

all: ${BINS}

${DIR_BIN}/% : ${DIR_SRC}/%.c ${DEPS} Makefile | ${DIR_BIN}
	${CC} $(filter %.c,$^) $(filter %.a,$^) -o $@ ${FLAGS} ${INCLUDES}

${DIR_BIN}:
	mkdir -p $@
