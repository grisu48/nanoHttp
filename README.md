nanoHttp
========

Really simple, small HTTP server.
Designed to act as a primitive HTTP server on my android devices.

For Android-Instructions, visit the Wiki: https://github.com/grisu48/nanoHttp/wiki/Android


CROSS-Compile for Android
=========================

Currently tested on my Cyanogenmod-11.0 Android Tablet. Compiled with
arm-linux-androideabi-gcc

To compile it, modify the Makefile. Replace CC with
CC=arm-linux-androideabi-gcc

and make sure "-static" is set in CC_FLAGS.

Multi-Threading
===============

Since 0.2 nanoHttp supports POSIX-threads. Each request is forked into a new
thread if desired to do so.
You can disable this feature by UNDEFINING the DEFINE _NANOHTTP_THREADING in
nanoHttp.c.
Since Android NDK supports POSIX-threads, this feature is Android-compatible.

Inspiration and original source
===============================

This project is build on the original source from sanos:
http://www.jbox.dk/sanos/webserver.htm
