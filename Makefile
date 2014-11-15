
#Use this to compile for android
#CC=arm-linux-androideabi-gcc
CC=gcc

# Flags and settings
CC_FLAGS=-static -Wall -pedantic -std=c99 -O3
LIBS=-pthread
OBJS=
CC_ENV=-D_POSIX_SOURCE
BIN=nanoHttp




default:	all
all:	$(BIN)

nanoHttp:	nanoHttp.c nanoHttp.h
	$(CC) $(CC_FLAGS) -o $@ $< $(CC_ENV) $(LIBS) 

