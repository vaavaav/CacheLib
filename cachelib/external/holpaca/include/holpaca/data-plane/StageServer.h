#pragma once

#include <holpaca/data-plane/ControlAPI.h>
#include <holpaca/data-plane/DataAPI.h>
#include <holpaca/protos/holpaca.grpc.pb.h>
#include <holpaca/protos/holpaca.pb.h>

namespace holpaca {
class Cache : public DataAPI, public ControlAPI {};
class StageServer : public holpaca::stage::Service {
  Cache* cache;

 public:
  StageServer(Cache* cache);
};
} // namespace holpaca