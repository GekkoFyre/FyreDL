###
 ##   Thank you for using "Gecho" for your encrypted, XMPP instant messages.
 ##   Copyright (C) 2015. GekkoFyre.
 ##
 ##
 ##   This program is free software: you can redistribute it and/or modify
 ##   it under the terms of the GNU Affero General Public License as
 ##   published by the Free Software Foundation, either version 3 of the
 ##   License, or (at your option) any later version.
 ##
 ##   This program is distributed in the hope that it will be useful,
 ##   but WITHOUT ANY WARRANTY; without even the implied warranty of
 ##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ##   GNU Affero General Public License for more details.
 ##
 ##   You should have received a copy of the GNU Affero General Public License
 ##   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ##
 ##
 ##   The latest source code updates can be obtained from [ 1 ] below at your
 ##   leisure. A web-browser or the 'git' application may be required.
 ##
 ##   [ 1 ] - https://github.com/GekkoFyre/gecho
 ##
 #################################################################################

# Try to find 'newt'
# The following definitions are very important and are defined once 'ncursestw' is found
#
# GkPugiXML_FOUND
# GkPugiXML_INCLUDE_DIR
# GkPugiXML_LIBRARY

include(LibFindMacros)

# Dependencies
# NOTE: What you need to find in /usr/lib
# libfind_package(GkNCurses ncurses)

INCLUDE(CMakeDetermineSystem)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux") # Check if we are on Linux
    # Use pkg-config to get hints about paths
    # NOTE: This is what you need to find in /usr/lib/pkgconfig
    libfind_pkg_check_modules(GkPugiXML_PKGCONF pugixml*)


    # Include dir
    find_path(GkPugiXML_INCLUDE_DIR
        NAMES "pugixml.hpp"
        HINTS "${GkPugiXML_PKGCONF_INCLUDE_DIRS}"
    )

    # Finally the library itself
    find_library(GkPugiXML_LIBRARY
        NAMES "pugixml"
        HINTS "${GkPugiXML_PKGCONF_LIBRARY_DIRS}"
    )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR "cygwin" OR "mingw") # Check if we are on Microsoft Windows
    message(SEND_ERROR "Microsoft Windows is not supported as of yet!")
else()
    message(SEND_ERROR "The current operating system is not supported. If you believe this is a bug or you would like support to be added, then please contact the developers.")
endif()

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(GkPugiXML_PROCESS_INCLUDES GkPugiXML_INCLUDE_DIR)
set(GkPugiXML_PROCESS_LIBS GkPugiXML_LIBRARY)
libfind_process(GkPugiXML)

