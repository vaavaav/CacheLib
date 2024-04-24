add_library (cachelib_holpaca
./PoolCache.h
./PoolCache.cc
./PoolCacheProxy.h
./PoolCacheProxy.cc
  )

add_dependencies(cachelib_holpaca thrift_generated_files)

add_executable (cachelib_holpaca_run test/RunCaches.cc)
target_link_libraries(cachelib_holpaca_run PUBLIC cachelib)
add_executable (cachelib_holpaca_test test/SendQueries.cc)
target_link_libraries(cachelib_holpaca_test PUBLIC cachelib_holpaca)

target_link_libraries(cachelib_holpaca PUBLIC
holpaca::holpaca Shards::Shards
)

install(
  TARGETS
     cachelib_holpaca_run cachelib_holpaca_test
  DESTINATION ${BIN_INSTALL_DIR}
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
