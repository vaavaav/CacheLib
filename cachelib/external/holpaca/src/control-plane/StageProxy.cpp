#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include <holpaca/control-plane/StageProxy.h>
#include <holpaca/protos/holpaca.grpc.pb.h>
#include <holpaca/protos/holpaca.pb.h>

#include <map>

namespace holpaca {
StageProxy& StageProxy::connect(const std::string& address) {
  m_channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  m_stage = holpaca::stage::NewStub(m_channel);
  return *this;
}

bool StageProxy::isConnected() const {
  return m_channel->GetState(true) == GRPC_CHANNEL_READY;
}

bool StageProxy::isValid() const {
  return m_stage != nullptr && m_stage != nullptr;
}

void StageProxy::resize(size_t newSize) {
  ClientContext ctx;
  ResizeRequest req;
  ResizeResponse rep;

  req.set_newsize(newSize);
  m_stage->Resize(&ctx, req, &rep);
}

Status StageProxy::getStatus() {
  ClientContext ctx;
  StatusRequest req;
  StatusResponse rep;

  m_stage->GetStatus(&ctx, req, &rep);

  return Status{
      rep.maxsize(),
      rep.hits(),
      rep.lookups(),
      std::map<uint64_t, uint32_t>{rep.tailaccesses().begin(),
                                   rep.tailaccesses().end()},
      std::map<uint64_t, double>{rep.mrc().begin(), rep.mrc().end()},
  };
}
} // namespace holpaca
