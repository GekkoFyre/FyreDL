[ GekkoFyre ] LevelDB branch
==================
Current version: 1.20

This is the forked project of [quasardb's](https://www.quasardb.net/) [LevelDB](http://code.google.com/p/leveldb/) branch which has modifications for Microsoft Windows support. It can be viewed [here](https://github.com/bureau14/leveldb), although it is seemingly no longer maintained and hence, the reason for the fork where [GekkoFyre](https://github.com/gekkofyre) is making additional improvements. Currently, it has patches provided by [Netzeband](https://github.com/Netzeband) at this [issue topic](https://github.com/google/leveldb/issues/475) and minor improvements provided by [phobos-dthorga](https://github.com/phobos-dthorga). In no way is this to be taken as an endorsement for Netzeband whom is unaffiliated with this project.

Currently, the featureset is the following:

* Full Windows support: everything builds, all tests pass;
* [CMake](http://www.cmake.org/) based build
* Explicit (thread unsafe) de-allocation routines for "clean exits". Helps a lot when running your application into a leak detector;
* The Windows build requires [Boost](http://www.boost.org/); 
* Our code is C++11ish and may require a recent compiler;
* Lots of warnings fixed;
* Is not 100% compliant with Google coding style.

Tested on [Arch Linux](https://www.archlinux.org/) and [Microsoft Windows](https://www.microsoft.com/) (32-bit and 64-bit).
