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

# Try to find 'leveldb'
#
# LEVELDB_FOUND
# LEVELDB_INCLUDE_DIR
# LEVELDB_LIBRARIES

find_library(LEVELDB_LIBRARIES
    NAMES "leveldb"
    HINTS "${LEVELDB_LOCATION}/lib" "${LEVELDB_LOCATION}/lib64" "${LEVELDB_LOCATION}/lib32"
    DOC "The main leveldb library"
)

# -----------------------------------------------------
# LEVELDB Include Directories
# -----------------------------------------------------
find_path(LEVELDB_INCLUDE_DIR
    NAMES "db.h"
    HINTS "${LEVELDB_LOCATION}" "${LEVELDB_LOCATION}/include/leveldb" "${LEVELDB_LOCATION}/include/*" "/usr/include/leveldb"
    DOC "The leveldb include directory"
)

if(LEVELDB_INCLUDE_DIR)
    message(STATUS "leveldb includes found in ${LEVELDB_INCLUDE_DIR}")
endif()

# -----------------------------------------------------
# Handle the QUIETLY and REQUIRED arguments and set LEVELDB_FOUND to TRUE if
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(leveldb DEFAULT_MSG LEVELDB_LIBRARIES LEVELDB_INCLUDE_DIR)
mark_as_advanced(LEVELDB_INCLUDE_DIR LEVELDB_LIBRARIES)

