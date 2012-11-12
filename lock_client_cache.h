// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"

#include <map>
#include <pthread.h>
// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
public:
	virtual void dorelease(lock_protocol::lockid_t) = 0;
	virtual ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
public:
	
	enum lock_status{NONE, FREE, LOCKED, ACQUIRING, RELEASING};
	typedef int lock_status_t;
private:
	class lock_release_user *lu;
	int rlock_port;
	std::string hostname;
	std::string id;
	
	std::map<lock_protocol::lockid_t, lock_status_t> locks_;
	pthread_mutex_t locks_mutex_;
	pthread_cond_t	locks_changed_;
	
	//pthread_mutex_t locks_remote_mutex_;
	//pthread_cond_t	locks_remote_cond_;
	
	//must be called with locks_mutex_ locked 
	lock_status_t get_lock_status(lock_protocol::lockid_t lid);
	
	//must be called with locks_mutex_ locked 
	bool lock(lock_protocol::lockid_t lid);
	//must be called with locks_mutex_ locked 
	bool unlock(lock_protocol::lockid_t lid);
public:

	lock_client_cache(std::string xdst, class lock_release_user *l = 0);
	virtual ~lock_client_cache() {};
	lock_protocol::status acquire(lock_protocol::lockid_t);
	lock_protocol::status release(lock_protocol::lockid_t);
	rlock_protocol::status revoke_handler(lock_protocol::lockid_t, int &);
	rlock_protocol::status retry_handler(lock_protocol::lockid_t, int &);
};


#endif