
DIR_SRC=src
DIR_BIN=bin

DEPS=glad/include/glad/glad.h

SRCS=$(wildcard ${DIR_SRC}/*.c)
BINS=$(foreach s,${SRCS},$(subst ${DIR_SRC}/,${DIR_BIN}/,${s:%.c=%}))

INCLUDES=-Iglad/include

all: ${DEPS} ${BINS}

${DIR_BIN}/% : ${DIR_SRC}/%.c ${DEPS} | ${DIR_BIN}
	gcc $(filter %.c %h,$^) -o $@ ${INCLUDES}

${DIR_BIN}:
	mkdir -p $@

%/glad.h : glad
	cd glad && python glad --generator=c --out-path .

glad:
	git clone https://github.com/Dav1dde/glad
