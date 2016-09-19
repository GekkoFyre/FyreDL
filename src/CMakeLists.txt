###
 ##   Thank you for using the "FyreDL" download manager for your HTTP(S)/
 ##   FTP(S)/BitTorrent downloads. You are looking at the source code to
 ##   make the application work and as such, it will require compiling
 ##   with the appropriate tools.
 ##   Copyright (C) 2016. GekkoFyre.
 ##
 ##
 ##   FyreDL is free software: you can redistribute it and/or modify
 ##   it under the terms of the GNU General Public License as published by
 ##   the Free Software Foundation, either version 3 of the License, or
 ##   (at your option) any later version.
 ##
 ##   FyreDL is distributed in the hope that it will be useful,
 ##   but WITHOUT ANY WARRANTY; without even the implied warranty of
 ##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ##   GNU General Public License for more details.
 ##
 ##   You should have received a copy of the GNU General Public License
 ##   along with FyreDL.  If not, see <http://www.gnu.org/licenses/>.
 ##
 ##
 ##   The latest source code updates can be obtained from [ 1 ] below at your
 ##   leisure. A web-browser or the 'git' application may be required.
 ##
 ##   [ 1 ] - https://github.com/GekkoFyre/fyredl
 ##
 #################################################################################

cmake_minimum_required(VERSION 3.2.0 FATAL_ERROR)
project(FyreDL C CXX) # http://stackoverflow.com/questions/15193785/how-to-get-cmake-to-recognize-pthread-on-ubuntu

set(CMAKE_VERBOSE_MAKEFILE ON)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# https://cmake.org/cmake/help/v3.0/prop_tgt/AUTOUIC.html
set(CMAKE_AUTOUIC ON)

# https://wiki.qt.io/Using_CMake_build_system
set(CMAKE_AUTORCC ON)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find the Qt libraries
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

set(SOURCE_FILES
    cmnroutines.hpp
    cmnroutines.cpp
    main.cpp
    mainwindow.hpp
    mainwindow.cpp)

# http://doc.qt.io/qt-5/cmake-manual.html
# http://www.executionunit.com/blog/2014/01/22/moving-from-qmake-to-cmake/
qt5_wrap_ui(UI_HEADERS
            mainwindow.ui)

qt5_add_resources(UI_RESOURCES assets.qrc)

# Create the main executable and name it depending on the operating system
set(EXE_NAME "fyredl" CACHE STRING "The name of the executable.")
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set_property(CACHE EXE_NAME PROPERTY STRINGS "fyredl")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR "mingw" OR "cygwin")
    set_property(CACHE EXE_NAME PROPERTY STRINGS "fyredl.exe")
endif()

add_executable("${EXE_NAME}" ${SOURCE_FILES} ${UI_HEADERS} ${UI_RESOURCES})
set_property(TARGET ${EXE_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${EXE_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

# http://stackoverflow.com/questions/10851247/how-to-activate-c-11-in-cmake
# https://cmake.org/cmake/help/v3.1/command/target_compile_features.html
# https://cmake.org/cmake/help/v3.1/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html#prop_gbl:CMAKE_CXX_KNOWN_FEATURES
# http://stackoverflow.com/questions/10984442/how-to-detect-c11-support-of-a-compiler-with-cmake/20165220#20165220
message(STATUS "Your C++ compiler supports the following features: ")
get_property(known_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
foreach(i ${known_features})
    message(STATUS "${i}")
endforeach()

# https://cmake.org/cmake/help/v3.1/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html
set(GK_NEEDED_FEATURES cxx_aggregate_default_initializers
                       cxx_auto_type
                       cxx_decltype
                       cxx_decltype_auto
                       cxx_inline_namespaces
                       cxx_long_long_type
                       cxx_nullptr
                       cxx_range_for
                       cxx_raw_string_literals
                       cxx_return_type_deduction
                       cxx_sizeof_member
                       cxx_strong_enums
                       cxx_static_assert
                       cxx_unicode_literals)
target_compile_features("${EXE_NAME}" PRIVATE ${GK_NEEDED_FEATURES})

#
# Generate the 'GetText' translation files
#
find_package(GettextTools REQUIRED)
if (GETTEXT_FOUND)
    set(stockDir "${CMAKE_CURRENT_SOURCE_DIR}/i18n")
    set(langDir "${CMAKE_CURRENT_BINARY_DIR}/lang")
    set(outDir "${CMAKE_CURRENT_BINARY_DIR}/locale")
    gettext_make_target("lang"
      HIERARCHY "{1}/{2}/{3}/{4}.mo"
      KEYWORDS  "_"
      DOMAIN    "gettexttools-dom"
      STOCK_DIR "${stockDir}"
      LANG_DIR  "${langDir}"
      OUT_DIR   "${outDir}"
      SOURCE    "${SOURCE_FILES}")
    list(APPEND makeCleanFiles ${outDir})
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "locale") # CLEANUP
    message(STATUS "Gettext processes successfully finished!")
else()
    message(SEND_ERROR "The 'Gettext' library could not be found!")
endif (GETTEXT_FOUND)

#
# Find the correct Boost C++ packages
#
set(Boost_USE_MULTITHREADED TRUE)
# set(Boost_USE_STATIC_LIBS TRUE)
# set(Boost_USE_STATIC_RUNTIME FALSE)
find_package(Boost 1.44.0 REQUIRED COMPONENTS "iostreams"
                                              "program_options"
                                              "date_time"
                                              "filesystem"
                                              "system")
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Boost_LIBRARIES})
    message(STATUS "Boost C++ libraries have been found.")
else()
    message(SEND_ERROR "The required 'Boost C++' libraries are either not installed, or not multithreaded and/or of not an up-to-date version.")
endif(Boost_FOUND)

#
# Find 'libpthreads'
#
find_package(Threads REQUIRED)
if (Threads_FOUND)
    set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif(Threads_FOUND)

#
# Find 'wget'
#
find_package(CURL REQUIRED)
if (CURL_FOUND)
    set(LIBS ${LIBS} ${CURL_LIBRARIES})
endif(CURL_FOUND)

#
# Optionally, find 'Doxygen' and generate the API documentation
#
# find_package(Doxygen REQUIRED)
# if (DOXYGEN_FOUND)
# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
# add_custom_target("api-doc" "${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}" COMMENT "Generating API documentation with Doxygen" VERBATIM)
# endif(DOXYGEN_FOUND)

IF(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "./debug/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "./debug/lib")
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/debug")
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin/debug")
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "./release/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "./release/lib")
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "./other-build/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "./other-build/lib")
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
ENDIF()

#
# Link the (static?) executable with the other, required external libraries
#
target_link_libraries(fyredl Qt5::Core Qt5::Widgets ${LIBS})

INCLUDE(CMakeDetermineSystem)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR "cygwin" OR "mingw") # Check if we are on Microsoft Windows
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        message(STATUS "Creating a Microsoft Windows DEBUG build.")
        # TODO: Determine if it's neccessary to add a DEBUG flag here!
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        message(STATUS "Creating a Microsoft Windows RELEASE build.")
    endif(CMAKE_BUILD_TYPE)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") # Check if we are using the Visual Studio compiler
        set (CMAKE_GENERATOR "Visual Studio")
        CHECK_INCLUDE_FILES("Windows.h" HAVE_WINDOWS_H)
        set_target_properties(gecho PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
        message(STATUS "Using the Microsoft Visual C++ compiler!")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # This is apparently not tested!
        message(STATUS "Using the GCC compiler!")
    else()
        message(SEND_ERROR "You are using an unsupported Microsoft Windows compiler! (Not MSVC or GCC)")
        # TODO: Add support for 'mingw'!
    endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux") # Check if we are on Linux
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
            # https://spin.atomicobject.com/2013/10/20/clang-compiler/
            # http://clang.llvm.org/docs/UsersManual.html#controlling-code-generation
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wconversion -Qunused-arguments -Wno-missing-braces -Wno-unused-parameter -g")
            message(STATUS "DEBUG build using the Clang compiler!")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            # http://stackoverflow.com/questions/3375697/useful-gcc-flags-for-c
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpointer-arith -Wcast-align -Wconversion -Wunreachable-code -g")
            message(STATUS "DEBUG build using the GCC compiler!")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
            message(STATUS "DEBUG build using the Intel compiler!")
        else()
            message(SEND_ERROR "You are using an unsupported Linux compiler! (Not GCC, Clang or Intel!)")
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion -Qno-unused-arguments -Wno-missing-braces -Wno-unused-parameter")
            message(STATUS "DEBUG build using the Clang compiler!")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            # http://stackoverflow.com/questions/3375697/useful-gcc-flags-for-c
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pointer-arith -Wno-cast-align -Wno-conversion -Wno-unreachable-code")
            message(STATUS "DEBUG build using the GCC compiler!")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
            message(STATUS "DEBUG build using the Intel compiler!")
        else()
            message(SEND_ERROR "You are using an unsupported Linux compiler! (Not GCC, Clang or Intel!)")
        endif()
    endif()
else()
    message(SEND_ERROR "Either you are using an unsupported platform or the platform configuration could not be detected! At this stage, we currently recommend Linux for the usage of this software application. If you believe this message is a bug, then please contact the developers.")
ENDIF()

#
# Tell CMake to install our binary into the 'bin' directory of the installation dir
#
# install(TARGETS "gecho" DESTINATION ${EXECUTABLE_OUTPUT_PATH})
