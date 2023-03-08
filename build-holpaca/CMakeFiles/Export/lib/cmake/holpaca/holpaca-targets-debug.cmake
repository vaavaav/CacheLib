#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "holpaca::spdlog" for configuration "Debug"
set_property(TARGET holpaca::spdlog APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(holpaca::spdlog PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/cmake/holpaca/libspdlogd.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS holpaca::spdlog )
list(APPEND _IMPORT_CHECK_FILES_FOR_holpaca::spdlog "${_IMPORT_PREFIX}/lib/cmake/holpaca/libspdlogd.a" )

# Import target "holpaca::holpaca" for configuration "Debug"
set_property(TARGET holpaca::holpaca APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(holpaca::holpaca PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libholpaca.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS holpaca::holpaca )
list(APPEND _IMPORT_CHECK_FILES_FOR_holpaca::holpaca "${_IMPORT_PREFIX}/lib/libholpaca.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
