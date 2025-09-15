PINE Reference Implementation
======

[![build](https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fci.govanify.com%2Fjob%2Fgovanify%2Fjob%2Fpine%2Fjob%2Fmaster)
![tests](https://img.shields.io/jenkins/tests?compact_message&jobUrl=https%3A%2F%2Fci.govanify.com%2Fjob%2Fgovanify%2Fjob%2Fpine%2Fjob%2Fmaster%2F)
![coverage](https://img.shields.io/jenkins/coverage/api?jobUrl=https%3A%2F%2Fci.govanify.com%2Fjob%2Fgovanify%2Fjob%2Fpine%2Fjob%2Fmaster%2F)
](https://ci.govanify.com/blue/organizations/jenkins/govanify%2Fpine/activity?branch=master)

PINE (Protocol for Instrumentation of Emulators) is an IPC protocol designed
for embedded devices emulators, specifically video game emulators.  
It has a small common subset of operations and additions that are platform
specific. No obligation is given to implement the common operations although it
is highly encouraged to aid in cross-compatibility of the protocol.
You'll find in this repository the [protocol standard](standard/)(currently as a draft) 
along with the reference client implementation.
The reference server implementation can be found in the [PCSX2](https://pcsx2.net) project.

The reference implementation you'll find here is written in C++, although
[bindings in popular languages are
available](https://code.govanify.com/govanify/pine/src/branch/master/bindings/).

A small C++ client example is provided along with the API. It can be compiled
by executing the command `meson build && cd build && ninja` in the folder
"example" that is included in the releases.  
If you want to run the tests you'll have to do 
`meson build && cd build && meson test`. This will require you to set
environment variables to correctly startup the emulator(s). Refer to `src/tests.cpp`
to see which ones. 

Meson and ninja ARE portable across OSes as-is and shouldn't require any tinkering. Please
refer to [the meson documentation](https://mesonbuild.com/Using-with-Visual-Studio.html) 
if you really want to use another generator, say, Visual Studio, instead of ninja.   

On Doxygen you can find the documentation of the C++ API [here](@ref PINE).  
The C API is documented [here](@ref bindings/c/c_ffi.h) and is probably what you
want to read if you use language bindings.


Have fun!  
-Gauvain "GovanifY" Roussel-Tarbouriech, 2020
