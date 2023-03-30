#pragma once
#include <atomic>
#include <holpaca/config.h>
#include <holpaca/data_plane/stage.h>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
#include <memory>
#include <thread>

namespace holpaca::data_plane {
    class DetachedStage : public Stage {
    };
}
