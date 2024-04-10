add_library (cachelib_holpaca
./PoolStage.h
./PoolStage.cc
  )

include(FetchContent)
FetchContent_Declare(
    Shards
    GIT_REPOSITORY https://github.com/vaavaav/SHARDS-cpp.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(Shards)

target_link_libraries(cachelib_holpaca PUBLIC
holpaca::holpaca Shards
)

install(TARGETS cachelib_holpaca Shards
        EXPORT cachelib-exports
        DESTINATION ${LIB_INSTALL_DIR} )

if (BUILD_SHARED_LIBS)
  install(
    TARGETS cachelib_holpaca Shards
    EXPORT cachelib-exports
    DESTINATION ${LIB_INSTALL_DIR}
  )
endif ()