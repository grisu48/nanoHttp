
#Use this to compile for android
#CC=arm-linux-androideabi-gcc
CC=gcc
CC_ANDROID=arm-linux-androideabi-gcc

# Flags and settings
CC_FLAGS=-Wall -Wextra -pedantic -std=c99 -O2
LIBS=-pthread
OBJS=
CC_ENV=-D_POSIX_C_SOURCE=200809L
BIN=nanoHttp




default:	all
all:	$(BIN)

nanoHttp:	nanoHttp.c nanoHttp.h
	$(CC) $(CC_FLAGS) -o $@ $< $(CC_ENV) $(LIBS) 

static:	nanoHttp.c nanoHttp.h
	$(CC) -static $(CC_FLAGS) -o $@ $< $(CC_ENV) $(LIBS) 
	
	
android: nanoHttp.c nanoHttp.h
	$(CC_ANDROID) -static $(CC_FLAGS) -o nanoHttp $< $(CC_ENV) $(LIBS) 
