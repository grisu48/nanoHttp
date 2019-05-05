nanoHttp
========

[![Build Status](https://travis-ci.org/grisu48/nanoHttp.svg?branch=master)](https://travis-ci.org/grisu48/nanoHttp)

Really simple, small HTTP server.
Designed to act as a primitive HTTP server on my android devices.

For Android-Instructions, visit the [Wiki](https://github.com/grisu48/nanoHttp/wiki/Android)

Binaries
========

Some binaries can be found in the bin/ folder.
If you need Android binaries, please keep in mind to use the right platform. You may need to compile the sources by your own!


CROSS-Compile for Android
=========================

This procedure has been tested on my Cyanogenmod-11.0 Android Tablet. Compiled with
`arm-linux-androideabi-gcc` (You will need a working [Android NDK toolchain](https://developer.android.com/ndk/guides/standalone_toolchain.html))

Compile for Android with

    make android

Make sure, the Android-NDK toolchain is installed and available as arm-linux-androideabi-gcc, otherwise modify the Makefile: Replace `CC` with `CC=arm-linux-androideabi-gcc`
(see Makefile)

and make sure `-static` is set in `CC_FLAGS` (in the Makefile)



For detailed Install instructions see the [Wiki](https://github.com/grisu48/nanoHttp/wiki/Android) or checkout the `Android-INSTALL` file in the android/ folder

Multi-Threading
===============

Since 0.2 nanoHttp supports POSIX-threads. Each request is forked into a new
thread if desired to do so.

You can disable this feature by removing the DEFINE `_NANOHTTP_THREADING` in
`nanoHttp.h`.
Since Android NDK supports POSIX-threads, this feature is Android-compatible.

Inspiration and original source
===============================

This project is build on the original source from [sanos](http://www.jbox.dk/sanos/webserver.htm)
