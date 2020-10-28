
# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to your build.

cmake_minimum_required(VERSION 3.4.1)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add_library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

add_library( # Specifies the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             # Associated headers in the same location as their source file are automatically included.
             src/main/cpp/native-lib.cpp

           )

# Specifies a path to native header files.
include_directories(src/main/cpp/include/)


# Searches for a specified prebuilt library and stores the path as a
# variable. Because system libraries are included in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Defines the name of the path variable that stores the location of the NDK library.
              log-lib

              # Specifies the name of the NDK library that you want CMake to locate.
              log
            )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in the
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       # OpenCV lib
                       #lib_opencv

                       # Links the target library to the log library included in the NDK.
                       ${log-lib}
                     )