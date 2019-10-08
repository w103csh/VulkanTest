
IF(NOT FMOD_DIR)
  SET(FMOD_DIR $ENV{FMOD_DIR})
ENDIF()

# Not sure how the "L" libraries are different
IF(FALSE)
  SET(FMOD_SUFFIX L)
ENDIF()
MESSAGE(STATUS "FMOD_SUFFIX: ${FMOD_SUFFIX}")

MESSAGE(STATUS "CMAKE_GENERATOR_PLATFORM: ${CMAKE_GENERATOR_PLATFORM}")
IF(WIN32)
  IF (${CMAKE_GENERATOR_PLATFORM} STREQUAL Win32)
    SET(FMOD_ARCH_DIR x86)
  ELSEIF(${CMAKE_GENERATOR_PLATFORM} STREQUAL x64)
    SET(FMOD_ARCH_DIR x64)
  ENDIF()
ENDIF()
MESSAGE(STATUS "FMOD_ARCH_DIR: ${FMOD_ARCH_DIR}")

FIND_PATH(FMOD_INCLUDE_DIR fmod.h
  HINTS
  ${FMOD_DIR}
  PATH_SUFFIXES
    api/core/inc
)
MESSAGE(STATUS "FMOD_INCLUDE_DIR: ${FMOD_INCLUDE_DIR}")

FIND_PATH(FMOD_LIBRARY_DIR fmod${FMOD_SUFFIX}.dll
  HINTS
  ${FMOD_DIR}
  PATH_SUFFIXES
    api/core/lib/${FMOD_ARCH_DIR}
)
MESSAGE(STATUS "FMOD_LIBRARY_DIR: ${FMOD_LIBRARY_DIR}")

FIND_LIBRARY(FMOD_LIBRARY fmod${FMOD_SUFFIX}_vc
  HINTS
  ${FMOD_DIR}
  PATH_SUFFIXES
    api/core/lib/${FMOD_ARCH_DIR}
)
MESSAGE(STATUS "FMOD_LIBRARY: ${FMOD_LIBRARY}")

# handle the QUIETLY and REQUIRED arguments and set FMOD_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FMOD DEFAULT_MSG
    FMOD_LIBRARY
    FMOD_INCLUDE_DIR
    FMOD_LIBRARY_DIR
)

IF (FMOD_FOUND)
    MESSAGE(STATUS "FMOD found!")
ELSE (FMOD_FOUND)
    MESSAGE(ERROR "FMOD not (entirely) found!")
ENDIF (FMOD_FOUND)

set(FMOD_INCLUDE_DIRS
    ${FMOD_INCLUDE_DIR}
)

macro(_FMOD_APPEND_LIBRARIES _list _release)
   set(_debug ${_release}_DEBUG)
   if(${_debug})
      set(${_list} ${${_list}} optimized ${${_release}} debug ${${_debug}})
   else()
      set(${_list} ${${_list}} ${${_release}})
   endif()
endmacro()

if(FMOD_FOUND)
   _FMOD_APPEND_LIBRARIES(FMOD_LIBRARIES FMOD_LIBRARY)
endif()