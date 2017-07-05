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

cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
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

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

# http://doc.qt.io/qt-5/cmake-manual.html
# Find the Qt libraries
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Charts REQUIRED)

if (Qt5Widgets_FOUND)
    if (Qt5Widgets_VERSION VERSION_LESS 5.7.0)
        message(FATAL_ERROR "Minimum supported Qt5 version is 5.70, due to the need for graphing
        libraries. Unless Qt5 is not installed on your system?")
    endif()
else()
    message(SEND_ERROR "The Qt5Widgets library could not be found!")
endif(Qt5Widgets_FOUND)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

set(SOURCE_FILES
        gui/about.hpp
        gui/about.cpp
        gui/addurl.hpp
        gui/addurl.cpp
        cmnroutines.hpp
        cmnroutines.cpp
        curl_easy.hpp
        curl_easy.cpp
        curl_multi.hpp
        curl_multi.cpp
        default_var.hpp
        dl_view.hpp
        dl_view.cpp
        main.cpp
        gui/mainwindow.hpp
        gui/mainwindow.cpp
        gui/settings.hpp
        gui/settings.cpp
        singleton_emit.hpp
        singleton_proc.hpp
        torrent/client.hpp
        torrent/client.cpp
        torrent/misc.hpp
        torrent/misc.cpp
        torrent/session.hpp
        torrent/session.cpp)

set(EXTERNAL_SOURCE_FILES
    ./../utils/fast-cpp-csv-parser/csv.h)

# http://www.executionunit.com/blog/2014/01/22/moving-from-qmake-to-cmake/
qt5_wrap_ui(UI_HEADERS
                gui/about.ui
                gui/mainwindow.ui
                gui/addurl.ui
                gui/settings.ui)

qt5_add_resources(UI_RESOURCES assets.qrc)

# Create the main executable and name it depending on the operating system
set(EXE_NAME "fyredl" CACHE STRING "The name of the executable.")
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set_property(CACHE EXE_NAME PROPERTY STRINGS "fyredl")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR "mingw" OR "cygwin")
    set_property(CACHE EXE_NAME PROPERTY STRINGS "fyredl.exe")
endif()

add_executable("${EXE_NAME}" ${SOURCE_FILES} ${EXTERNAL_SOURCE_FILES} ${UI_HEADERS} ${UI_RESOURCES})
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
set(GK_NEEDED_FEATURES cxx_auto_type
                       cxx_decltype
                       cxx_decltype_auto
                       cxx_digit_separators
                       cxx_explicit_conversions
                       cxx_inline_namespaces
                       cxx_lambdas
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
# Find the correct Boost C++ packages
#
INCLUDE(CMakeDetermineSystem)
set(Boost_USE_MULTITHREADED TRUE)
# set(Boost_USE_STATIC_LIBS FALSE CACHE BOOL "Whether to use static libraries with regard to Boost C++ or not")
# set(Boost_USE_STATIC_RUNTIME FALSE)
find_package(Boost 1.54.0 REQUIRED COMPONENTS "iostreams"
                                              "filesystem"
                                              "system")
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Boost_LIBRARIES})
    message(STATUS "Boost C++ libraries have been found.")
    add_definitions(-DBOOST_ALL_NO_LIB)

    if (${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR "cygwin" OR "mingw")
        # http://stackoverflow.com/questions/9742003/platform-detection-in-cmake
        add_definitions(-D_WIN32_WINNT=NTDDI_VISTASP1)
    endif()
else()
    message(SEND_ERROR "The required 'Boost C++' libraries are either not installed, or not multithreaded and/or of not an up-to-date version. Boost C++ 1.54.0 is the minimally required version.")
endif(Boost_FOUND)

#
# Find 'libpthreads'
#
find_package(Threads REQUIRED)
if (Threads_FOUND)
    set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif(Threads_FOUND)

#
# Find 'libtorrent'
#
find_package(LibtorrentRasterbar REQUIRED)
if (LibtorrentRasterbar_FOUND)
    set(LIBS ${LIBS} ${LibtorrentRasterbar_LIBRARIES})
endif(LibtorrentRasterbar_FOUND)

#
# Find 'libcurl'
#
find_package(CURL REQUIRED)
if (CURL_FOUND)
    set(LIBS ${LIBS} ${CURL_LIBRARIES})
endif(CURL_FOUND)

#
# Find 'pugixml'
#
find_package(PugiXML REQUIRED)
if (PUGIXML_FOUND)
    include_directories(${PUGIXML_INCLUDE_DIR})
    set(LIBS ${LIBS} ${PUGIXML_LIBRARIES})
else()
    message(SEND_ERROR "The 'pugixml' library could not be found!")
endif(PUGIXML_FOUND)

#
# Qt5 translation routines
# http://stackoverflow.com/questions/19193121/automated-translation-management-with-cmake-and-qt5
# http://stackoverflow.com/questions/24095800/qt-internationalization-and-cmake-how-to-update-ts-and-dont-lose-them
# https://gist.github.com/02JanDal/70a39f2cc3d2002b3588
# https://github.com/JPNaude/dev_notes/wiki/Using-Google-Translate-to-translate-your-Qt-application
#
# file(GLOB TRANSLATION_FILES ${CMAKE_CURRENT_LIST_DIR}/*.ts)
# qt5_create_translation(TRANSLATION_MESSAGES ${FILES_TO_TRANSLATE} ${TRANSLATION_FILES})
# qt5_add_translation(TRANSLATION_QM ${TRANSLATION_FILES})
# add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES})
# add_custom_target(translations DEPENDS ${TRANSLATION_QM})

#
# Optionally, find 'Doxygen' and generate the API documentation
#
set(BUILD_API_DOC FALSE CACHE BOOL "This determines if you want to build the doxygen documentation or not")
if (BUILD_API_DOC)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)
        add_custom_target("api-doc" "${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_BINARY_DIR}" COMMENT "Generating API documentation with Doxygen" VERBATIM)
    else()
        message(SEND_ERROR "Could not find Doxygen, despite '-DBUILD_API_DOC' being set to true! Maybe disable this variable?")
    endif(DOXYGEN_FOUND)
endif(BUILD_API_DOC)

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

target_link_libraries(fyredl Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Charts ${LIBS})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR "cygwin" OR "mingw") # Check if we are on Microsoft Windows
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        message(STATUS "Creating a Microsoft Windows DEBUG build.")
        # TODO: Determine if it's neccessary to add a DEBUG flag here!
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        message(STATUS "Creating a Microsoft Windows RELEASE build.")
    endif()
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") # Check if we are using the Visual Studio compiler
        set(CMAKE_GENERATOR "Visual Studio")
        set_target_properties(fyredl PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN")
        message(STATUS "Using the Microsoft Visual C++ compiler!")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows -WIN32_LEAN_AND_MEAN -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-deprecated-declarations") # This is apparently not tested!
        message(STATUS "Using either the GCC or Clang compiler!")
    else()
        message(SEND_ERROR "You are using an unsupported Microsoft Windows compiler! (Not MSVC or GCC)")
    endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux") # Check if we are on Linux
    install(FILES ${TRANSLATION_QM} DESTINATION translations)
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
            # https://spin.atomicobject.com/2013/10/20/clang-compiler/
            # http://clang.llvm.org/docs/UsersManual.html#controlling-code-generation
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++ -Wall -Wextra -Werror -Wconversion -Qunused-arguments -Wno-missing-braces -Wno-unused-parameter -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-deprecated-declarations -g")
            message(STATUS "DEBUG build using the Clang compiler!")

            set(GK_ADDR_SANITIZE FALSE CACHE BOOL "See the Clang AddressSanitizer documentation for more details.")
            if(GK_ADDR_SANITIZE)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address")
            endif(GK_ADDR_SANITIZE)

            set(GK_MEM_SANITIZE FALSE CACHE BOOL "See the Clang MemorySanitizer documentation for more details.")
            if(GK_MEM_SANITIZE)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=memory")
            endif(GK_MEM_SANITIZE)

            set(GK_THREAD_SANITIZE FALSE CACHE BOOL "See the Clang ThreadSanitizer documentation for more details.")
            if(GK_THREAD_SANITIZE)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fsanitize=thread")
            endif(GK_THREAD_SANITIZE)
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            # http://stackoverflow.com/questions/3375697/useful-gcc-flags-for-c
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpointer-arith -Wcast-align -Wconversion -Wunreachable-code -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-deprecated-declarations -Wno-unused-parameter -g")
            message(STATUS "DEBUG build using the GCC compiler!")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
            message(STATUS "DEBUG build using the Intel compiler!")
            # TODO: Setup this section for the Intel compiler toolset!
        else()
            message(SEND_ERROR "You are using an unsupported Linux compiler! (Not GCC, Clang or Intel!)")
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion -Qno-unused-arguments -Wno-missing-braces -Wno-unused-parameter -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-deprecated-declarations")
            message(STATUS "DEBUG build using the Clang compiler!")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            # http://stackoverflow.com/questions/3375697/useful-gcc-flags-for-c
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pointer-arith -Wno-cast-align -Wno-conversion -Wno-unreachable-code -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-deprecated-declarations")
            message(STATUS "DEBUG build using the GCC compiler!")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
            message(STATUS "DEBUG build using the Intel compiler!")
            # TODO: Setup this section for the Intel compiler toolset!
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

