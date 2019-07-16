# Copyright (C) 2019  Christian Berger
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

###########################################################################
# Find libyami.
FIND_PATH(YAMI_INCLUDE_DIR
          NAMES YamiC.h
          PATHS /usr/local/include/libyami/
                /usr/include/libyami/)
MARK_AS_ADVANCED(YAMI_INCLUDE_DIR)
FIND_LIBRARY(YAMI_LIBRARY
             NAMES yami
             PATHS ${LIBYAMIDIR}/lib/
                    /usr/lib/x86_64-linux-gnu/
                    /usr/local/lib64/
                    /usr/lib64/
                    /usr/lib//)
MARK_AS_ADVANCED(YAMI_LIBRARY)

###########################################################################
IF (YAMI_INCLUDE_DIR
    AND YAMI_LIBRARY)
    SET(YAMI_FOUND 1)
    SET(YAMI_LIBRARIES ${YAMI_LIBRARY})
    SET(YAMI_INCLUDE_DIRS ${YAMI_INCLUDE_DIR})
ENDIF()

MARK_AS_ADVANCED(YAMI_LIBRARIES)
MARK_AS_ADVANCED(YAMI_INCLUDE_DIRS)

IF (YAMI_FOUND)
    MESSAGE(STATUS "Found libyami: ${YAMI_INCLUDE_DIRS}, ${YAMI_LIBRARIES}")
ELSE ()
    MESSAGE(STATUS "Could not find libyami")
ENDIF()
