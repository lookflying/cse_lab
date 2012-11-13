// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstddef>

lock_server::lock_server():
  nacquire_ (0)
{
    VERIFY(pthread_mutex_init(&lock_map_mutex_, NULL) ==0 );
    VERIFY(pthread_cond_init(&lock_cond_, NULL) == 0);
}

lock_server::~lock_server(){
    VERIFY(pthread_mutex_destroy(&lock_map_mutex_) == 0);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire_;
  return ret;
}

lock_protocol::status lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r){
    lock_protocol::status ret = lock_protocol::OK;

//    printf("client %d acquire lock %ld\n", clt, lid);

    pthread_mutex_lock(&lock_map_mutex_);
    if (!get_lock(clt, lid)){
        while (true){
            pthread_cond_wait(&lock_cond_, &lock_map_mutex_);
//            printf("client %d acquire lock %ld\n", clt, lid);
            if (get_lock(clt, lid))
                break;
        }
    }
    pthread_mutex_unlock(&lock_map_mutex_);

//    printf("client %d acquire lock %ld success\n", clt, lid);

    r = 0;
    return ret;
}

lock_protocol::status lock_server::release(int clt, lock_protocol::lockid_t lid, int &r){
    lock_protocol::status ret = lock_protocol::OK;
    printf("client %d release %lld\n", clt, lid);

    pthread_mutex_lock(&lock_map_mutex_);
    if (drop_lock(clt, lid)){
        r = 0;
        pthread_cond_signal(&lock_cond_);
    }else{
        r = -1;
        ret = lock_protocol::RETRY;
    }
    pthread_mutex_unlock(&lock_map_mutex_);
    if (r == 0){
        printf("client %d release %lld success\n", clt, lid);
    }else{
        printf("client %d release %lld fail\n", clt, lid);
    }
    return ret;
}

//critical
bool lock_server::get_lock(int clt, lock_protocol::lockid_t lid){
    bool rst = true;
    lock_map_iterator it = lock_map_.find(lid);
    if (it == lock_map_.end()){
        lock_map_[lid] = clt;
    }else{
        rst = false;
    }
    return rst;
}

//critical area
bool lock_server::drop_lock(int clt, lock_protocol::lockid_t lid){
    bool rst = true;
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
    return rst;
}
