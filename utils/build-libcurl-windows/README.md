Auto download and compile libcurl
=================================
This batch script will automatically download the latest libcurl source code and build it with the Visual Studio C/C++ compiler, using the x86_64 architecture.

Please note that this has been forked from [***blackrosezy***](https://github.com/blackrosezy/build-libcurl-windows), who was the original author of this project. [phobos-dthorga](https://github.com/phobos-dthorga) has added support for later versions of *Microsoft Visual Studio* and updated the included *wget* binary accordingly, so that it is capable of downloading the [libcurl](https://curl.haxx.se/libcurl/) source code without the HTTPS errors of the older *wget* binary.

Supported Visual Studio are:
*  Visual Studio 2008
*  Visual Studio 2010
*  Visual Studio 2012
*  Visual Studio 2013
*  Visual Studio 2015
* Visual Studio 2017 (support added by [phobos-dthorga](https://github.com/phobos-dthorga))


*Note-1*: All version of **Visual Studio express are unsupported**.

*Note-2*: This script is using third-party open source software
* `bin/7-zip` http://www.7-zip.org/
* `bin/unxutils` http://unxutils.sourceforge.net/
* `bin/xidel` http://www.videlibri.de/xidel.html
* `bin/wget` https://www.gnu.org/software/wget/ (see the included *LICENSES* folder for the additional libraries that were required)

Usage :

    $ build.bat

Output :

```
third-party
└───libcurl
    ├───include
    │   └───curl
    │           curl.h
    │           curlbuild.h
    │           curlrules.h
    │           curlver.h
    │           easy.h
    │           mprintf.h
    │           multi.h
    │           stdcheaders.h
    │           typecheck-gcc.h
    │
    └───lib
        ├───dll-debug-x64
        │       libcurl_debug.dll
        │       libcurl_debug.lib
        │       libcurl_debug.pdb
        │
        ├───dll-debug-x86
        │       libcurl_debug.dll
        │       libcurl_debug.lib
        │       libcurl_debug.pdb
        │
        ├───dll-release-x64
        │       libcurl.dll
        │       libcurl.lib
        │       libcurl.pdb
        │
        ├───dll-release-x86
        │       libcurl.dll
        │       libcurl.lib
        │       libcurl.pdb
        │
        ├───static-debug-x64
        │       libcurl_a_debug.lib
        │
        ├───static-debug-x86
        │       libcurl_a_debug.lib
        │
        ├───static-release-x64
        │       libcurl_a.lib
        │
        └───static-release-x86
                libcurl_a.lib
```

## FAQ
If you get message something like below, please re-run build.bat again.

    **** Retrieving:http://curl.haxx.se/download.html ****
    Downloading latest curl...
    http://curl.haxx.seAn unhandled exception occurred at $004C7D39 :: Bad port number.

License (build.bat)
-----------

    The MIT License (MIT)
    
    Copyright (c) 2014 Mohd Rozi
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
