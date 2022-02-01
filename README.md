# Deprecated

This is an old implementation of Umi which used Boost::fiber. It has been preceeded by a newer version which uses our own implementation of fibers, Tamashii.

Eventhough a new branch could have been created for this version, it has been decided to create a new repository. The number of changes is too great to consider the new Umi a simple version change, thus a new start was deemed more appropiate.

https://github.com/NetPunkSoftware/Umi


# Build

Using CMake:

    mkdir build
    cd build
    cmake ..
    make

Executable will be found at

    * build/samples/client/src/
    * build/samples/server/src/


## Tested with

* Clang 11.0 and libc++. 
* Visual Studio 2019 16.5.5 and 16.7.0 Preview 4

Should work with any compile as long as it has support for

* Conceps TS
* C++17 major features (notably *constexpr*, *fold expressions*, *variant*)
* Boost 1.73 libraries

Might require tweaking *src/CMakeLists.txt* to add flags when not using the above combination.
