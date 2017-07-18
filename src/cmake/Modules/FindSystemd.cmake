if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    include(FindPkgConfig)
    pkg_check_modules(SYSTEMD "systemd")
    if (NOT SYSTEMD_FOUND)
        find_path(
                SYSTEMD_INCLUDE_DIRS
                NAMES systemd/sd-daemon.h
                DOC "Include directory for systemd"
        )

        find_library(
                SYSTEMD_LIBRARIES
                NAMES systemd
                DOC "Library for systemd"
        )

        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(SYSTEMD REQUIRED_VARS SYSTEMD_LIBRARIES SYSTEMD_INCLUDE_DIRS)

        if(SYSTEMD_FOUND)
            message(STATUS "systemd has been found!")
            set(SYSTEMD_LIBRARY ${SYSTEMD_LIBRARIES})
            set(SYSTEMD_INCLUDE_DIR ${SYSTEMD_INCLUDE_DIRS})
        endif(SYSTEMD_FOUND)
    endif()
    mark_as_advanced(SYSTEMD_FOUND SYSTEMD_LIBRARIES SYSTEMD_INCLUDE_DIRS)
endif()