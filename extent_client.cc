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

extent_protocol::status extent_client::get_cache(extent_protocol::extentid_t eid, std::string &buf){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	std::map<extent_protocol::extentid_t, std::string>::iterator it;
	it = data_cache_.find(eid);
	if (it == data_cache_.end()){
		ret = cl->call(extent_protocol::get, eid, buf);
		if (ret == extent_protocol::OK){
			data_cache_[eid] = buf;
			cache_states_[eid].dirty_ = false;
		}else{
			tprintf("ret != ok in %s\n", __FUNCTION__);
		}
	}else{
		buf = data_cache_[eid];
	}	
	attr_cache_[eid].atime = static_cast<unsigned int>(time(NULL));
	pthread_mutex_unlock(&cache_mutex_);
	return ret;
}

extent_protocol::status extent_client::get_attr_cache(extent_protocol::extentid_t eid, extent_protocol::attr &attr){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	std::map<extent_protocol::extentid_t, extent_protocol::attr>::iterator it;
	it = attr_cache_.find(eid);
	if (it == attr_cache_.end()){
		ret = cl->call(extent_protocol::getattr, eid, attr);
		if (ret == extent_protocol::OK){
			attr_cache_[eid] = attr;
		}else{
			tprintf("ret != ok in %s\n", __FUNCTION__);
		}
	}else{
		attr = attr_cache_[eid];
		tprintf("attr from cache %llu\n", eid);
	}
	tprintf("get attr cache %llu\n", eid);
	pthread_mutex_unlock(&cache_mutex_);
	return ret;
}

extent_protocol::status extent_client::put_cache(extent_protocol::extentid_t eid, std::string buf){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	std::map<extent_protocol::extentid_t, std::string>::iterator it;
	it = data_cache_.find(eid);
	if (it == data_cache_.end()){
		int r = 0;
		ret = cl->call(extent_protocol::put, eid, buf, r);
		if (ret == extent_protocol::OK){
			data_cache_[eid] = buf;
			cache_states_[eid].dirty_ = false;
			attr_cache_[eid].mtime = static_cast<unsigned int>(time(NULL));
		}else{
			tprintf("ret != ok in %s\n", __FUNCTION__);
		}
	}else{
		data_cache_[eid] = buf;
		cache_states_[eid].dirty_ = true;
		attr_cache_[eid].mtime = static_cast<unsigned int>(time(NULL));
		attr_cache_[eid].size = buf.size();
	}
	pthread_mutex_unlock(&cache_mutex_);
	return ret;
}

extent_protocol::status extent_client::put_cache_new(extent_protocol::extentid_t eid, std::string buf, int &rid){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	ret = cl->call(extent_protocol::put, eid, buf, rid);
	if (ret == extent_protocol::OK){
		data_cache_[static_cast<extent_protocol::extentid_t>(rid)] = buf;
		cache_states_[eid].dirty_ = false;
	}else{
		tprintf("ret != ok in %s\n", __FUNCTION__);
	}
	tprintf("put new %llu\n", eid);
	pthread_mutex_unlock(&cache_mutex_);
	return ret;
}

extent_protocol::status extent_client::remove_cache(extent_protocol::extentid_t eid){
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_mutex_);
	int r = 0;
	ret = cl->call(extent_protocol::remove, eid, r);
	if (ret == extent_protocol::OK){
		if (data_cache_.find(eid) != data_cache_.end()){
			data_cache_.erase(eid);
		}
		if (attr_cache_.find(eid) != attr_cache_.end()){
			attr_cache_.erase(eid);
		}
		if (cache_states_.find(eid) != cache_states_.end()){
			cache_states_.erase(eid);
		}
	}else{
		tprintf("ret != ok in %s\n", __FUNCTION__);
	}
	pthread_mutex_unlock(&cache_mutex_);
	return ret;
}


//public interface

extent_protocol::status extent_client::get(extent_protocol::extentid_t eid, std::string &buf){
	return get_cache(eid, buf);
}

extent_protocol::status extent_client::getattr(extent_protocol::extentid_t eid, extent_protocol::attr &attr){
	return get_attr_cache(eid, attr);
}

extent_protocol::status extent_client::put(extent_protocol::extentid_t eid, std::string buf){
	return put_cache(eid, buf);
}

extent_protocol::status extent_client::put(extent_protocol::extentid_t eid, std::string buf, int &rid){
	return put_cache_new(eid, buf, rid);
}


extent_protocol::status extent_client::remove(extent_protocol::extentid_t eid){
	return remove_cache(eid);
}


