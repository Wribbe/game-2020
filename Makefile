
DIR_SRC=src
DIR_BIN=bin

SRCS=$(wildcard ${DIR_SRC}/*.c)
BINS=$(foreach s,${SRCS},$(subst ${DIR_SRC}/,${DIR_BIN}/,${s:%.c=%}))

DEPS=\
	glad/include/glad/glad.h \
	glad/src/glad.c \
	glfw/include/GLFW/glfw3.h \
	glfw/src/libglfw3.a \

INCLUDES=-Iglad/include -Iglfw/include
FLAGS = -g -Wall -Werror -std=c11 -lGL -ldl -lX11 -lpthread -lm

all: ${DEPS} ${BINS}

${DIR_BIN}/% : ${DIR_SRC}/%.c ${DEPS} Makefile | ${DIR_BIN}
	gcc $(filter %.c %.a,$^) -o $@ ${FLAGS} ${INCLUDES}

${DIR_BIN}:
	mkdir -p $@

glad/% : | glad
	cd glad && python glad --generator=c --out-path .

glfw/% : | glfw
	cd glfw && git checkout latest && cmake . && make

glad:
	git clone https://github.com/Dav1dde/glad

glfw:
	git clone https://github.com/glfw/glfw
