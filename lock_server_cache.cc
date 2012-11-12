// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"

static void* reversed_rpc(void * lsc_ptr){
	lock_server_cache *lsc = (lock_server_cache*)lsc_ptr;
	lsc->reversed_rpc_daemon();
	return (void*)0;
}



lock_server_cache::lock_server_cache()
{
	VERIFY(pthread_mutex_init(&locks_mutex_, NULL) == 0);
	VERIFY(pthread_cond_init(&locks_cond_, NULL) == 0);
	VERIFY(pthread_mutex_init(&waiting_list_mutex_, NULL) == 0);
	VERIFY(pthread_cond_init(&waiting_list_cond_, NULL) == 0);
	//alive_ = true;
	//pthread_create(&reversed_rpc_thread_, NULL, reversed_rpc, (void*)this);
}

lock_server_cache::~lock_server_cache(){
//	alive_ = false;
//	pthread_cancel(reversed_rpc_thread_);

}

void lock_server_cache::reversed_rpc_daemon(){
	if (alive_){
		pthread_mutex_lock(&waiting_list_mutex_);
		while(alive_ ){
			pthread_cond_wait(&waiting_list_cond_, &waiting_list_mutex_);
			while(!revoke_queue_.empty()){
			}

		}
		pthread_mutex_unlock(&waiting_list_mutex_);	
	}
	
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
		waiting_list_[lid].push(id);
		VERIFY(reversed_rpc(rlock_protocol::revoke, locks_[lid], lid) == rlock_protocol::OK);
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
	}else{
		std::queue<std::string>* queue = &waiting_list_[lid];
		while(!queue->empty()){
			VERIFY(reversed_rpc(rlock_protocol::retry, queue->front(), lid) == rlock_protocol::OK);
			queue->pop();
		}

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

lock_protocol::status lock_server_cache::reversed_rpc(unsigned int proc, std::string cid, lock_protocol::lockid_t lid){
	handle h(cid);
	lock_protocol::status ret;
	if (h.safebind()){
		int r;
		ret = h.safebind()->call(proc, lid, r);
	}
	return ret;
}
