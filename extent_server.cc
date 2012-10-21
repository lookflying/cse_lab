// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include <cstdlib>
extent_server::extent_server() {
    VERIFY(pthread_mutex_init(&mutex_, NULL) == 0);

}



int extent_server::put(extent_protocol::extentid_t id, std::string buf, int & r)
{
	bool old = true;
    pthread_mutex_lock(&mutex_);
    if ((id & 0x7fffffff) == 0){//new need to reply generated id;
        old = false;

        do{
#if RAND_MAX == 2147483647
            id = (rand() & 0x7fffffff) | (id & 0x80000000);
#else
            id = (((rand() & 0x7fff) << 16) | (rand() & 0xffff)) | (id & 0x80000000);
#endif
        }while(attr_.find(id) != attr_.end());

    }else if (attr_.find(id) != attr_.end()){
		old = false;
	}
     r = static_cast<int>(id);
	data_[id] = buf;
    unsigned int t = static_cast<unsigned int>(time(NULL));
	if (!old){
		attr_[id].ctime = t;
	}
    attr_[id].atime = 0;
	attr_[id].mtime = t;
    attr_[id].size = buf.size();
    pthread_mutex_unlock(&mutex_);
    return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
    bool found = false;
    pthread_mutex_lock(&mutex_);
    if (attr_.find(id) != attr_.end()){
        found = true;
    }
    if (found){
        buf = data_[id];
        attr_[id].atime = static_cast<unsigned int>(time(NULL));
    }
    pthread_mutex_unlock(&mutex_);
    return found? extent_protocol::OK: extent_protocol::NOENT;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
    bool found = false;
    pthread_mutex_lock(&mutex_);
    if (attr_.find(id) != attr_.end()){
        found = true;
    }
    if (found){
        a = attr_[id];
    }
    pthread_mutex_unlock(&mutex_);
    return found? extent_protocol::OK: extent_protocol::NOENT;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
   bool found = false;
   pthread_mutex_lock(&mutex_);
   if (attr_.find(id) != attr_.end()){
       found = true;
   }
   if (found){
       attr_.erase(id);
       data_.erase(id);
   }
   pthread_mutex_unlock(&mutex_);
   return found? extent_protocol::OK: extent_protocol::NOENT;
}

