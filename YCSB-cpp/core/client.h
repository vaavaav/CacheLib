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
#include "db.h"
#include "core_workload.h"
#include "utils.h"
#include "countdown_latch.h"

namespace ycsbc {

inline int ClientThread(ycsbc::DB *db, ycsbc::CoreWorkload *wl, const int num_ops, bool is_loading,
                        bool init_db, bool cleanup_db, CountDownLatch *latch) {
    try {
        if (init_db) {
            db->Init();
        }

        int ops = 0;
        if(is_loading){
            for (; ops < num_ops; ++ops) {
                wl->DoInsert(*db);
            }
        } else {
            for (; ops < num_ops; ++ops) {
                if(wl->is_stop_requested()){
                    break;
                }
                wl->DoTransaction(*db);
            }
        }

        if (cleanup_db) {
            db->Cleanup();
        }

        latch->CountDown();
        return ops;
    } catch(const utils::Exception& e) {
        std::cerr<<"Caught exception: "<<e.what()<<std::endl;
        exit(1);
    }
}

} // ycsbc

#endif // YCSB_C_CLIENT_H_
