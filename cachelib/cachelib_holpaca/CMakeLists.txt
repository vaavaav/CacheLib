add_library (cachelib_holpaca
./PoolCache.h
./PoolCache.cc
./PoolCacheProxy.h
./PoolCacheProxy.cc
  )

add_dependencies(cachelib_holpaca thrift_generated_files)

add_executable (cachelib-holpaca-run-pool-caches RunPoolCaches.cc)
target_link_libraries(cachelib-holpaca-run-pool-caches PUBLIC cachelib)

target_link_libraries(cachelib_holpaca PUBLIC
holpaca::holpaca Shards::Shards
)

install(
  TARGETS
     cachelib-holpaca-run-pool-caches
  DESTINATION ${BIN_INSTALL_DIR}
)

install(TARGETS cachelib_holpaca
        EXPORT cachelib-exports
        DESTINATION ${LIB_INSTALL_DIR} )