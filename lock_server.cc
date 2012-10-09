// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstddef>

lock_server::lock_server():
  nacquire (0)
{
    pthread_mutex_init(&lock_map_mutex_, NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r){
    lock_protocol::status ret = lock_protocol::OK;
    while (true){
        if (get_lock(clt, lid))
            break;
        sleep(1);
    }
    r = 0;
    return ret;
}

lock_protocol::status lock_server::release(int clt, lock_protocol::lockid_t lid, int &r){
    lock_protocol::status ret = lock_protocol::OK;
    if (drop_lock(clt, lid)){
        r = 0;
    }else{
        r = -1;
    }
    return ret;
}

bool lock_server::get_lock(int clt, lock_protocol::lockid_t lid){
    bool rst = true;
    pthread_mutex_lock(&lock_map_mutex_);
    lock_map_iterator it = lock_map_.find(lid);
    if (it == lock_map_.end()){
        lock_map_[lid] = clt;
    }else{
        rst = false;
    }
    pthread_mutex_unlock(&lock_map_mutex_);
    return rst;
}

bool lock_server::drop_lock(int clt, lock_protocol::lockid_t lid){
    bool rst = true;
    pthread_mutex_lock(&lock_map_mutex_);
    lock_map_iterator it = lock_map_.find(lid);
    if (it == lock_map_.end()){
        rst = false;
    }else{
        if ((*it).second == clt){
            lock_map_.erase(it);
        }else{
            rst = false;
        }
    }
    pthread_mutex_unlock(&lock_map_mutex_);
    return rst;
}
