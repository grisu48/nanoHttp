
#Use this to compile for android
#CC=arm-linux-androideabi-gcc

CC=gcc
CC_FLAGS=-static -Wall -pedantic -std=c99
LIBS=
OBJ=
CC_ENV=-D_POSIX_SOURCE
BIN=nanoHttp

default:	all
all:	$(BIN)

nanoHttp:	nanoHttp.c
	$(CC) $(CC_FLAGS) -o $@ $< $(LIBS) $(CC_ENV)
