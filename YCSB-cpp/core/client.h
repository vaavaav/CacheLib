//
//  client.h
//  YCSB-cpp
//
//  Copyright (c) 2020 Youngjae Lee <ls4154.lee@gmail.com>.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_CLIENT_H_
#define YCSB_C_CLIENT_H_

#include <string>

#include "core_workload.h"
#include "countdown_latch.h"
#include "terminator_thread.h"
#include "db.h"
#include "utils.h"

namespace ycsbc {

inline int ClientThread(std::chrono::seconds sleepafterload,
                        std::chrono::seconds maxexecutiontime,
                        int threadId,
                        ycsbc::DB* db,
                        ycsbc::CoreWorkload* wl,
                        const int num_ops,
                        bool is_loading,
                        bool init_db,
                        bool cleanup_db,
                        CountDownLatch* latch) {
  try {
    db->SetThreadId(threadId);
    if (init_db) {
      db->Init();
    }
    if (sleepafterload.count() > 0) {
      std::this_thread::sleep_for(sleepafterload);
    }
    std::future<void> terminator;
    if (maxexecutiontime.count() > 0) {
      terminator = std::async(std::launch::async, ycsbc::TerminatorThread,
                              maxexecutiontime, wl);
    }

    int ops = 0;
    if (is_loading) {
      for (; ops < num_ops; ++ops) {
        wl->DoInsert(*db);
      }
    } else {
      db->Active();
      for (; ops < num_ops; ++ops) {
        if (wl->is_stop_requested()) {
          break;
        }
        wl->DoTransaction(*db);
      }
      db->Inactive();
    }

    if (cleanup_db) {
      db->Cleanup();
    }

    latch->CountDown();
    if (maxexecutiontime.count() > 0) {
      terminator.wait();
    }

    return ops;
  } catch (const utils::Exception& e) {
    std::cerr << "Caught exception: " << e.what() << std::endl;
    exit(1);
  }
}

} // namespace ycsbc

#endif // YCSB_C_CLIENT_H_
