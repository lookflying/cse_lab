// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  rpcs *rlsrpc = new rpcs(0);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  const char *hname;
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlsrpc->port();
  id = host.str();

  VERIFY(pthread_mutex_init(&locks_mutex_, NULL) == 0);
  VERIFY(pthread_cond_init(&locks_changed_, NULL) == 0);
//  VERIFY(pthread_mutex_init(&locks_remote_mutex_, NULL) == 0);
//  VERIFY(pthread_cond_init(&locks_remote_changed_, NULL) == 0);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
	pthread_mutex_lock(&locks_mutex_);
	if (!lock(lid)){
		while(true){
			pthread_cond_wait(&locks_changed_, &locks_mutex_);
			if (lock(lid)){
				break;
			}
		}
	}
	pthread_mutex_unlock(&locks_mutex_);
	return lock_protocol::OK;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
	lock_protocol::status ret = lock_protocol::OK;
	pthread_mutex_lock(&locks_mutex_);
	if (unlock(lid)){
		pthread_cond_signal(&locks_changed_);
	}else{
		ret = lock_protocol::RETRY;
	}
	pthread_mutex_unlock(&locks_mutex_);
	return ret;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, int &)
{
	int ret = rlock_protocol::OK;
	pthread_mutex_lock(&locks_mutex_);
	locks_[lid] = RELEASING;
	pthread_mutex_unlock(&locks_mutex_);
	return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, int &)
{
	int ret = rlock_protocol::OK;
	pthread_mutex_lock(&locks_mutex_);
	pthread_cond_broadcast(&locks_changed_);
	pthread_mutex_unlock(&locks_mutex_);
	return ret;
}


lock_client_cache::lock_status_t 
lock_client_cache::get_lock_status(lock_protocol::lockid_t lid){

	if (locks_.find(lid) == locks_.end()){
		locks_[lid] = NONE;
	}
	return locks_[lid];
}

bool lock_client_cache::lock(lock_protocol::lockid_t lid){
	bool ret = false;
	bool missed = false;
	lock_status_t status = get_lock_status(lid);
	switch(status){
	case FREE:
		locks_[lid] = LOCKED;
		ret = true;
		break;
	case LOCKED:
	case ACQUIRING:
	case RELEASING:
		break;
	case NONE:
		missed = true;
		break;
	}

	if (missed){
//		locks_[lid] = ACQUIRING;
		lock_protocol::status rst;
		int r;
		rst = cl->call(lock_protocol::acquire, lid, id, r);
		if (rst == lock_protocol::OK){
			locks_[lid] = LOCKED;
			ret = true;
		}else if (rst == lock_protocol::RETRY){
			//
		}
	}
	return ret;
}

bool lock_client_cache::unlock(lock_protocol::lockid_t lid){
	bool ret = false;
	bool revoked = false;
	lock_status_t status = get_lock_status(lid);
	switch(status){
	case LOCKED:
		locks_[lid] = FREE;
		ret = true;
		break;
	case RELEASING:
		revoked = true;
		break;
	case FREE:
	case ACQUIRING:
	case NONE:
		//should not be here
		VERIFY(0);
		break;
	}

	if (revoked){
		lock_protocol::status rst;
		int r;
		rst = cl->call(lock_protocol::release, lid, id, r);
		if (rst == lock_protocol::OK){
			locks_[lid] = NONE;
			ret = true;
		}
	}
	return ret;
}
