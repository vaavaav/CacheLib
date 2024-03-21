#pragma once
#include <grpcpp/server.h>
#include <holpaca/Cache.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace holpaca {
class StageConfig {
  std::string m_logFile;
  Cache* m_cache;
  std::string m_serviceAddress;
  std::string m_registrationAddress;

 public:
  StageConfig& setLogger(std::string logFile);
  StageConfig& setServiceAddress(std::string address);
  StageConfig& setRegistrationAddress(std::string address);
  StageConfig& setCache(Cache* cache);
  StageConfig& fromFile(std::string& configFile);
  bool isValid() const;

  friend class Stage;
};
} // namespace holpaca