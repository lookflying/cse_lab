// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <tprintf.h>
// The calls assume that the caller holds a lock on the extent
extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
  VERIFY(pthread_mutex_init(&cache_mutex_, NULL) == 0);
}


//new private cache method
void extent_client::update_cache(extent_protocol::extentid_t eid, std::string buf){
	data_cache_[eid] = buf;

	unsigned int t = static_cast<unsigned int>(time(NULL));
	attr_cache_[eid].atime = t;
	attr_cache_[eid].mtime = t;
	attr_cache_[eid].ctime = t;
	attr_cache_[eid].size = buf.size();

	cache_states_[eid].update();	
}

bool extent_client::access_cache(extent_protocol::extentid_t eid, std::string &buf){
	if (cache_states_[eid].is_valid()){
		buf = data_cache_[eid];
		attr_cache_[eid].atime = static_cast<unsigned int>(time(NULL));
		return true;
	}else{
		return false;
	}
}


//in case that data is not in cache
//return false when that happens
bool extent_client::modify_cache(extent_protocol::extentid_t eid, std::string buf){
	if (cache_states_[eid].is_valid()){
		data_cache_[eid] = buf;
		unsigned int t = static_cast<unsigned int>(time(NULL));
		attr_cache_[eid].atime = t;
		attr_cache_[eid].mtime = t;
		attr_cache_[eid].ctime = t;	
		attr_cache_[eid].size = buf.size();
		cache_states_[eid].modify();
		return true;
	}else{
		return false;
	}
}

bool extent_client::access_attr_cache(extent_protocol::extentid_t eid, extent_protocol::attr &attr){
	if (cache_states_[eid].is_attr_valid()){
		attr = attr_cache_[eid];
		return true;
	}else{
		return false;
	}
}

void extent_client::update_attr_cache(extent_protocol::extentid_t eid, extent_protocol::attr attr){
	attr_cache_[eid] = attr;

	cache_states_[eid].update_attr();
}


void extent_client::flush_cache(extent_protocol::extentid_t eid){

}

//in case that data is not in cache
//return false when that happens
bool extent_client::delete_cache(extent_protocol::extentid_t eid){
	if (cache_states_[eid].is_valid()){
		unsigned int t = static_cast<unsigned int>(time(NULL));
		attr_cache_[eid].atime = t;
		attr_cache_[eid].mtime = t;
		
		cache_states_[eid].del();
		return true;
	}else{
		return false;
	}
}
//public interface

extent_protocol::status extent_client::get(extent_protocol::extentid_t eid, std::string &buf){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	if (access_cache(eid, buf)){
		goto release;
	}else{
		ret = cl->call(extent_protocol::get, eid, buf);
		if (ret == extent_protocol::OK){
			update_cache(eid, buf);
		}
		goto release;
	}
release:
	pthread_mutex_unlock(&cache_mutex_);
	return ret;
}


extent_protocol::status extent_client::getattr(extent_protocol::extentid_t eid, extent_protocol::attr &attr){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	if (access_attr_cache(eid, attr)){
		goto release;
	}else{
		ret = cl->call(extent_protocol::getattr, eid, attr);
		if (ret == extent_protocol::OK){
			update_attr_cache(eid, attr);
		}
		goto release;
	}
release:
	pthread_mutex_unlock(&cache_mutex_);
	return ret;
}


extent_protocol::status extent_client::put(extent_protocol::extentid_t eid, std::string buf){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	if (modify_cache(eid, buf)){
		goto release;
	}else{
		std::string in_buf;
		ret = cl->call(extent_protocol::get, eid, in_buf);
		if (ret == extent_protocol::OK){
			update_cache(eid, in_buf);
			VERIFY(modify_cache(eid, buf));
		}
		goto release;
	}
release:
	pthread_mutex_unlock(&cache_mutex_);
	return ret;
}

extent_protocol::status extent_client::put(extent_protocol::extentid_t eid, std::string buf, int &rid){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	ret = cl->call(extent_protocol::put, eid, buf, rid);
	if (ret == extent_protocol::OK){
		update_cache(static_cast<extent_protocol::extentid_t>(rid), buf);
		goto release;
	}else{
		goto release;
	}
release:
	pthread_mutex_unlock(&cache_mutex_);
	return ret;
}


extent_protocol::status extent_client::remove(extent_protocol::extentid_t eid){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	if (delete_cache(eid)){
		goto release;
	}else{
		std::string in_buf;
		ret = cl->call(extent_protocol::get, eid, in_buf);
		if (ret == extent_protocol::OK){
			update_cache(eid, in_buf);
			VERIFY(delete_cache(eid));
		}
		goto release;
	}
release:
	pthread_mutex_unlock(&cache_mutex_);
	return ret;	
}


