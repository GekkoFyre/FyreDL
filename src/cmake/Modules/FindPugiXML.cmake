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

# Try to find 'pugixml'
#
# PUGIXML_FOUND
# PUGIXML_INCLUDE_DIR
# PUGIXML_LIBRARIES

find_library(PUGIXML_LIBRARIES
    NAMES "pugixml"
    HINTS "${PUGIXML_LOCATION}/lib" "${PUGIXML_LOCATION}/lib64" "${PUGIXML_LOCATION}/lib32"
    DOC "The main pugixml library"
)

# -----------------------------------------------------
# PUGIXML Include Directories
# -----------------------------------------------------
find_path(PUGIXML_INCLUDE_DIR 
    NAMES "pugixml.hpp"
    HINTS "${PUGIXML_LOCATION}" "${PUGIXML_LOCATION}/include" "${PUGIXML_LOCATION}/include/*" "/usr/include"
    DOC "The pugixml include directory"
)

if(PUGIXML_INCLUDE_DIR)
    message(STATUS "pugixml includes found in ${PUGIXML_INCLUDE_DIR}")
endif()

# -----------------------------------------------------
# Handle the QUIETLY and REQUIRED arguments and set PUGIXML_FOUND to TRUE if 
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pugixml DEFAULT_MSG PUGIXML_LIBRARIES PUGIXML_INCLUDE_DIR)
mark_as_advanced(PUGIXML_INCLUDE_DIR PUGIXML_LIBRARIES)

