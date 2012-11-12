// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"

static void* reverse_rpc(void *){
	return (void*)0;
}
lock_server_cache::lock_server_cache()
{
	VERIFY(pthread_mutex_init(&locks_mutex_, NULL) == 0);
	VERIFY(pthread_cond_init(&locks_cond_, NULL) == 0);
	VERIFY(pthread_mutex_init(&waiting_list_mutex_, NULL) == 0);
	VERIFY(pthread_cond_init(&waiting_list_cond_, NULL) == 0);
	
}

bool lock_server_cache::get_lock(std::string cid, lock_protocol::lockid_t lid){
	bool ret = true;
	locks_iterator_t it = locks_.find(lid);
	if (it == locks_.end()){
		locks_[lid] = cid;
		ret = true;
	}else{
		ret = false;
	}
	return ret;
}

bool lock_server_cache::drop_lock(std::string cid, lock_protocol::lockid_t lid){
	bool ret = true;
	locks_iterator_t it = locks_.find(lid);
	if (it == locks_.end()){
		ret = false;
	}else{
		if (cid.compare(it->second) == 0){
			locks_.erase(it);
			ret = true;
		}else{
			ret = false;
		}
	}
	return ret;
}
int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, int &)
{
	lock_protocol::status ret = lock_protocol::OK;
	pthread_mutex_lock(&locks_mutex_);
	if (!get_lock(id, lid)){
		ret = lock_protocol::RETRY;
	}
	pthread_mutex_unlock(&locks_mutex_);
	return ret;
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, int &r)
{
	lock_protocol::status ret = lock_protocol::OK;
	pthread_mutex_lock(&locks_mutex_);
	if (!drop_lock(id, lid)){
		ret = lock_protocol::RETRY; 
	}
	pthread_mutex_unlock(&locks_mutex_);
	return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
	tprintf("stat request\n");
	r = nacquire;
	return lock_protocol::OK;
}

