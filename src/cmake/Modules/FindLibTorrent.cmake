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

# Try to find 'libtorrent'
#
# LIBTORRENT_FOUND
# LIBTORRENT_INCLUDE_DIR
# LIBTORRENT_LIBRARIES

find_library(LIBTORRENT_LIBRARIES
    NAMES "libtorrent-rasterbar" "torrent-rasterbar"
    HINTS "${LIBTORRENT_LOCATION}/lib" "${LIBTORRENT_LOCATION}/lib64" "${LIBTORRENT_LOCATION}/lib32" "/usr/local/lib" "/usr/local/lib64"
    DOC "The main libtorrent library"
)

# -----------------------------------------------------
# LIBTORRENT Include Directories
# -----------------------------------------------------
find_path(LIBTORRENT_INCLUDE_DIR 
    NAMES "torrent.hpp"
    HINTS "${LIBTORRENT_LOCATION}" "${LIBTORRENT_LOCATION}/include" "${LIBTORRENT_LOCATION}/include/*" "/usr/include" "/usr/local/include/*"
    DOC "The libtorrent include directory"
)

if(LIBTORRENT_INCLUDE_DIR)
    message(STATUS "torrent includes found in ${LIBTORRENT_INCLUDE_DIR}")
endif()

# -----------------------------------------------------
# Handle the QUIETLY and REQUIRED arguments and set LIBTORRENT_FOUND to TRUE if 
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(torrent DEFAULT_MSG LIBTORRENT_LIBRARIES LIBTORRENT_INCLUDE_DIR)
mark_as_advanced(LIBTORRENT_INCLUDE_DIR LIBTORRENT_LIBRARIES)

