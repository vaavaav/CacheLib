add_library(cachelib_holpaca_data
  ./Stage.h
  ./CacheAllocator.h
  ./Stage.cpp
  ./CacheAllocator.cpp
  ./CacheAllocatorConfig.h
)

target_link_libraries(cachelib_holpaca_data PUBLIC
  cachelib_allocator
  cachelib_holpaca_proto
  flows::flows
)

install(TARGETS cachelib_holpaca_data
        EXPORT cachelib-exports
        DESTINATION ${LIB_INSTALL_DIR} )