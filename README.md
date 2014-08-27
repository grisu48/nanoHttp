nanoHttp
========

Really simple, small HTTP server.
Designed to act as a primitive HTTP server on my android devices.

Inspiration and original source
===============================

This project is build on the original source from sanos:
http://www.jbox.dk/sanos/webserver.htm


CROSS-Compile for Android
=========================

Currently tested on my Cyanogenmod-11.0 Android Tablet. Compiled with
arm-linux-androideabi-gcc

To compile it, modify the Makefile. Replace CC with
CC=arm-linux-androideabi-gcc

and make sure "-static" is set in CC_FLAGS.
