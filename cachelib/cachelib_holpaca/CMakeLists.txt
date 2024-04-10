add_library (cachelib_holpaca
./PoolCache.h
./PoolCache.cc
  )

target_link_libraries(cachelib_holpaca PUBLIC
holpaca::holpaca Shards::Shards
)

install(TARGETS cachelib_holpaca
        EXPORT cachelib-exports
        DESTINATION ${LIB_INSTALL_DIR} )

if (BUILD_SHARED_LIBS)
  install(
    TARGETS cachelib_holpaca 
    EXPORT cachelib-exports
    DESTINATION ${LIB_INSTALL_DIR}
  )
endif ()