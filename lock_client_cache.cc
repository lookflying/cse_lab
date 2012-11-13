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
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
	pthread_mutex_lock(&locks_mutex_);
	if (!lock(lid)){
		while(true){
			lock_cond_wait(lid, locks_mutex_);
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
		lock_cond_broadcast(lid);
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
	lock_status_t status;
	pthread_mutex_lock(&locks_mutex_);
	tprintf("%s got revoke %lld\n", id.c_str(), lid);
	status = get_lock_status(lid);
	if (status == LOCKED){
		set_lock_status(lid, RELEASING);
		lock_cond_broadcast(lid);
	}else if (status == FREE){
		ret = rlock_protocol::OK_FREE;
		set_lock_status(lid, NONE);
		tprintf("%s released lock %lld\n", id.c_str(), lid);
	}
	pthread_mutex_unlock(&locks_mutex_);
	return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, int &)
{
	int ret = rlock_protocol::OK;
	pthread_mutex_lock(&locks_mutex_);
	lock_cond_broadcast(lid);
	pthread_mutex_unlock(&locks_mutex_);
	tprintf("%s got retry %lld\n", id.c_str(), lid);
	return ret;
}


lock_client_cache::lock_status_t 
lock_client_cache::get_lock_status(lock_protocol::lockid_t lid){
	if (locks_.find(lid) == locks_.end()){
		locks_[lid].status_ = NONE;
		VERIFY(pthread_cond_init(&locks_[lid].cond_, NULL) == 0);
	}
	return locks_[lid].status_;
}

bool lock_client_cache::is_lock_owner(lock_protocol::lockid_t lid,  pthread_t owner){
	lock_status_t status;
	bool ret = false;
	status = get_lock_status(lid);
	switch(status){
	case FREE:
	case NONE:
		break;
	case LOCKED:
	case ACQUIRING:
	case RELEASING:
		ret = pthread_equal(locks_[lid].owner_, owner);
		break;
	default:
		//should never be here
		VERIFY(false);
		break;
	}
	return ret;
}

void lock_client_cache::set_lock_status(lock_protocol::lockid_t lid, lock_status_t status){
	locks_[lid].status_ = status;
}

void lock_client_cache::set_lock_owner(lock_protocol::lockid_t lid, pthread_t owner){
	locks_[lid].owner_ = owner;
}

bool lock_client_cache::lock(lock_protocol::lockid_t lid){
	bool ret = false;
	bool missed = false;
	lock_status_t status = get_lock_status(lid);
	switch(status){
	case FREE:
		set_lock_status(lid, LOCKED);
		set_lock_owner(lid, pthread_self());
		ret = true;
		break;
	case LOCKED:
		break;
	case ACQUIRING:
		if (is_lock_owner(lid, pthread_self())){
			missed = true;
		}
		break;
	case RELEASING:
		break;
	case NONE:
		missed = true;
		set_lock_status(lid, ACQUIRING);
		set_lock_owner(lid, pthread_self());
		break;
	}


	if (missed){
		lock_protocol::status rst;
		int r;
		rst = cl->call(lock_protocol::acquire, lid, id, r);
		if (rst == lock_protocol::OK){
			set_lock_status(lid, LOCKED);
			set_lock_owner(lid, pthread_self());
			ret = true;
		}else if (rst == lock_protocol::RETRY){
			//
		}
		tprintf("%s lock %lld %s\n", id.c_str(), lid, ret?"success":"fail");
	}
//	tprintf("%s lock %lld", id.c_str(), lid);
//	printf(" %s\n", ret?"success":"fail");
	return ret;
}

bool lock_client_cache::unlock(lock_protocol::lockid_t lid){
	
	bool ret = false;
	bool revoked = false;
	lock_status_t status = get_lock_status(lid);
	switch(status){
	case LOCKED:
		if (is_lock_owner(lid, pthread_self())){
			set_lock_status(lid, FREE);
			ret = true;
		}
		break;
	case RELEASING:
		if (is_lock_owner(lid, pthread_self())){
			revoked = true;
		}
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
		int i = 0;
		do{
			rst = cl->call(lock_protocol::release, lid, id, r);
			tprintf("%s try release lock %lld for %d time\n", id.c_str(), lid, ++i);
		}while(rst != lock_protocol::OK);
		set_lock_status(lid, NONE);
		ret = true;
		tprintf("%s unlock %lld %s \n", id.c_str(), lid, ret?"success":"fail");
	}
	
//	tprintf("%s unlock %lld", id.c_str(), lid);
//	printf(" %s\n", ret?"success":"fail");
	return ret;
}
