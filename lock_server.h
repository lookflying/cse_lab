// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <map>

#include <pthread.h>
typedef std::map<lock_protocol::lockid_t, int>::iterator lock_map_iterator;
class lock_server {

protected:
    int nacquire;
    pthread_mutex_t lock_map_mutex_;
    std::map<lock_protocol::lockid_t, int> lock_map_;

public:
    lock_server();
    ~lock_server();
    lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
    lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
    lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
private:
    bool get_lock(int clt, lock_protocol::lockid_t lid);
    bool drop_lock(int clt, lock_protocol::lockid_t lid);
};

#endif 







