
DIR_SRC=src
DIR_BIN=bin

GIT_DEPS=glad

SRCS=$(wildcard ${DIR_SRC}/*.c)
BINS=$(foreach s,${SRCS},$(subst ${DIR_SRC}/,${DIR_BIN}/,${s:%.c=%}))

all: ${GIT_DEPS} ${BINS}

${DIR_BIN}/% : ${DIR_SRC}/%.c | ${DIR_BIN} ${GIT_DEPS}
	gcc $^ -o $@

${DIR_BIN}:
	mkdir -p $@

glad/include/glad/glad.h : glad
	cd glad && python __main__.py --generator=c --out-path .

glad:
	git clone https://github.com/Dav1dde/glad
