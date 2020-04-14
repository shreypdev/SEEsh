# build an executable named myprog from myprog.c
CC = gcc
CFLAGS = -Wall -g

all: SEEsh.c 
	${CC} ${CFLAGS} -o SEEsh SEEsh.c
clean:
	rm -f SEEsh SEEsh.o
run: SEEsh
	./SEEsh