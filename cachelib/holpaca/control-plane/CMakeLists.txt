add_library(cachelib_holpaca_control
./Controller.h
./Controller.cpp
./ControllerConfig.h
./CacheProxy.h
./CacheProxy.cpp
./ProxyManager.h
./ProxyManager.cpp
./algorithms/ControlAlgorithm.h
./algorithms/HitRatioMaximization.h
./algorithms/HitRatioMaximization.cpp
)

target_link_libraries(cachelib_holpaca_control PUBLIC
  cachelib_holpaca_proto
)

install(TARGETS cachelib_holpaca_control
        EXPORT cachelib-exports
        DESTINATION ${LIB_INSTALL_DIR} )

add_executable(cachelib_holpaca_controller
  ./Main.cpp
)

target_link_libraries(cachelib_holpaca_controller PUBLIC
  cachelib_holpaca_control
)

install(TARGETS cachelib_holpaca_controller
        EXPORT cachelib-exports
        DESTINATION ${BIN_INSTALL_DIR} )