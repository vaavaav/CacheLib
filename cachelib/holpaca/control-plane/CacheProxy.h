#pragma once

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <chrono>

#include "cachelib/holpaca/Cache.h"
#include "cachelib/holpaca/protos/Holpaca.grpc.pb.h"
#include "cachelib/holpaca/protos/Holpaca.pb.h"

namespace facebook {
namespace cachelib {
namespace holpaca {
class CacheProxy : public Cache {
  std::unique_ptr<::holpaca::Stage::Stub> m_stub;
  std::chrono::nanoseconds m_lastKeepAlive;

 public:
  CacheProxy(const std::string& address, std::chrono::nanoseconds timestamp);
  void keepAlive(std::chrono::nanoseconds timestamp);
  bool isAlive() const;

  void resize(std::unordered_map<int32_t, uint64_t> newSizes) override final;
  std::unordered_map<int32_t, PoolStatus> getStatus() override final;
};
} // namespace holpaca
} // namespace cachelib
} // namespace facebook