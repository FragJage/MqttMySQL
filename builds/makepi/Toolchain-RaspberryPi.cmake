# Target system
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_CROSSCOMPILE TRUE)

# specify the cross compiler
# upload on https://sourceforge.net/projects/raspberry-pi-cross-compilers/
# other project on https://github.com/Pro/raspi-toolchain
SET(CMAKE_C_COMPILER /home/rpi/cross-pi-gcc-9.3.0-0/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER /home/rpi/cross-pi-gcc-9.3.0-0/bin/arm-linux-gnueabihf-g++)
set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH /home/rpi/rootfs)
SET(CMAKE_SYSROOT /home/rpi/rootfs)
include_directories(${CMAKE_SYSROOT}/usr/local/include)
link_directories(${CMAKE_SYSROOT}/usr/local/lib)

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,-rpath-link=${CMAKE_SYSROOT}/lib" CACHE INTERNAL "" FORCE)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

SET(THREADS_PTHREAD_ARG "0" CACHE STRING "Result from TRY_RUN" FORCE)

