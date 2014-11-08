
#Use this to compile for android
CC=arm-linux-androideabi-gcc
#CC=gcc

# Flags and settings
CC_FLAGS=-static -Wall -pedantic -std=c99
LIBS=-pthread
OBJ=
CC_ENV=-D_POSIX_SOURCE
BIN=nanoHttp




default:	all
all:	$(BIN)

nanoHttp:	nanoHttp.c
	$(CC) $(CC_FLAGS) -o $@ $< $(LIBS) $(CC_ENV)
