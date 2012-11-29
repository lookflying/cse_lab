// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
	lc = new lock_client_cache(lock_dst);
	ec = new extent_client(extent_dst);
	std::string buf;
	extent_protocol::extentid_t rootid = 0x00000001;
	lock(static_cast<inum>(rootid));
	extent_protocol::status s = ec->get(rootid, buf);
	if (s == extent_protocol::NOENT){
//      std::ostringstream rootcontent;
//      rootcontent << '.';
//      rootcontent << delimiter;
//      rootcontent << rootid << entry_delimiter;
//      VERIFY(ec->put(rootid, rootcontent.str()) == extent_protocol::OK);
	VERIFY(ec->put(rootid, std::string()) == extent_protocol::OK);
	}else{
		VERIFY(s == extent_protocol::OK);
	}
	unlock(static_cast<inum>(rootid));

}

yfs_client::~yfs_client(){
	delete lc;
	delete ec;
}

void yfs_client::lock(inum i_num){
	lc->acquire(static_cast<lock_protocol::lockid_t>(i_num));
}

void yfs_client::unlock(inum i_num){
	lc->release(static_cast<lock_protocol::lockid_t>(i_num));	
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::name(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

std::string yfs_client::to_dir_entry(std::string name, inum i_num){
    std::ostringstream buf;
    buf << name;
    buf << delimiter;
    buf << i_num;
    return buf.str();
}

void yfs_client::to_name_inum(std::string dir_entry, std::string &name, inum &i_num){
    size_t pos = dir_entry.find_last_of(delimiter);
    name = dir_entry.substr(0, pos);
    std::istringstream is(dir_entry.substr(pos + 1));
    is >>i_num;
}

void yfs_client::get_dir_entries(std::string content, std::map<std::string, inum> &entries){
    size_t last_pos = 0;
    fflush(stdout);
    for (size_t pos = content.find_first_of(entry_delimiter);
         pos < content.size();
         pos = content.find_first_of(entry_delimiter, pos + 1))
    {
        std::string name;
        inum i_num;
        to_name_inum(content.substr(last_pos, pos - last_pos), name, i_num);
        entries[name] = i_num;
        last_pos = pos + 1;
    }
}

void yfs_client::get_dir_content(std::map<std::string, inum> entries, std::string &content){
    std::map<std::string, inum>::iterator it;
    std::ostringstream os;
    for (it = entries.begin(); it != entries.end(); it++){
        os << to_dir_entry(it->first, it->second);
        os << entry_delimiter;
    }
    content = os.str();
}



bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
	int r = OK;
  // You modify this function for Lab 3
  // - hold and release the file lock

	printf("getfile %016llx\n", inum);
	lock(inum);
	extent_protocol::attr a;
	if (ec->getattr(inum, a) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}

	fin.atime = a.atime;
	fin.mtime = a.mtime;
	fin.ctime = a.ctime;
	fin.size = a.size;
	printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
	unlock(inum);
	return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
	int r = OK;
  // You modify this function for Lab 3
  // - hold and release the directory lock
	printf("getdir %016llx\n", inum);
	lock(inum);
	extent_protocol::attr a;
	if (ec->getattr(inum, a) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	din.atime = a.atime;
	din.mtime = a.mtime;
	din.ctime = a.ctime;

release:
	unlock(inum);
	return r;
}

int yfs_client::look_up_by_name(inum parent, std::string name, inum &i_num){
    int ret;
	int r;
    std::map<std::string, inum> entries;
    std::string content;
	lock(parent);
    if (!isdir(parent) && parent != 0x000000001){
        i_num = 0;
        r = NOENT;
		goto release;
    }
    if ((ret = ec->get(static_cast<extent_protocol::extentid_t>(parent), content)) != extent_protocol::OK){
		r = IOERR;
		goto release;
	}
    get_dir_entries(content, entries);
    if (entries.find(name) != entries.end()){
        i_num = entries[name];
        r = OK;
    }else{
        i_num = 0;
		r = NOENT;
    }
release:
	unlock(parent);
	return r;
}

int yfs_client::create_file(inum parent, std::string name, inum &i_num){
    i_num = 0x80000000;
    int r = static_cast<int>(i_num);
	int ret = OK;
    std::string buf;
	lock(parent);
    if (ec->put(static_cast<extent_protocol::extentid_t>(i_num), std::string(), r) != extent_protocol::OK){
        ret = IOERR;
		goto release;
	}
    i_num = static_cast<inum>(static_cast<unsigned int>(r));
    if (i_num == 0x80000000){
        ret =  IOERR;
		goto release;
    }else{
        if (ec->get(static_cast<extent_protocol::extentid_t>(parent), buf) != extent_protocol::OK){
            ret = NOENT;
			goto release;
        }
        buf += (to_dir_entry(name, i_num) + entry_delimiter);
        if (ec->put(static_cast<extent_protocol::extentid_t>(parent), buf) != extent_protocol::OK){
            ret = IOERR;
			goto release;
        }
    }
release:
	unlock(parent);
	return ret;
}

int yfs_client::mkdir(inum parent, std::string name, inum &i_num){
	i_num = 0x00000000;
	int r = static_cast<int>(i_num);
	int ret = OK;
	std::string buf;
	lock(parent);
	if (ec->put(static_cast<extent_protocol::extentid_t>(i_num), std::string(), r) != extent_protocol::OK){
		ret = IOERR;
		goto release;
	}
	i_num = static_cast<inum>(static_cast<unsigned int>(r));
	if (i_num == 0x00000000){
		ret = IOERR;
		goto release;
	}else{
		if (ec->get(static_cast<extent_protocol::extentid_t>(parent), buf) != extent_protocol::OK){
			ret = NOENT;
			goto release;
		}
		buf += (to_dir_entry(name, i_num) + entry_delimiter);
		if (ec->put(static_cast<extent_protocol::extentid_t>(parent), buf) != extent_protocol::OK){
			ret = IOERR;
			goto release;
		}
	}
release:
	unlock(parent);
	return ret;
}

int yfs_client::read_file(inum i_num, std::string &buf, off_t off, size_t &size){
    int ret = yfs_client::OK;
    std::string temp;
	lock(i_num);
    if (ec->get(static_cast<extent_protocol::extentid_t>(i_num), temp) != extent_protocol::OK){
        ret = IOERR;
		goto release;
	}
    buf = temp.substr(off, size);
    size = buf.size();
release:
	unlock(i_num);
    return ret;
}

int yfs_client::write_file(inum i_num, std::string buf, off_t off, size_t &size){
    int ret = yfs_client::OK;
    std::string temp;
    size_t had; 
	lock(i_num);
    if (ec->get(static_cast<extent_protocol::extentid_t>(i_num), temp) != extent_protocol::OK){
		ret = IOERR;
		goto release;
	}
    had = temp.size();
    if (had < static_cast<size_t>(off)){
        temp += std::string(off - had, '\0');
        temp += buf;
    }else if( had > static_cast<size_t>(off) + size){
        temp.replace(off, size, buf);
    }else{
        temp = temp.substr(0, off) + buf;
    }
    if (ec->put(static_cast<extent_protocol::extentid_t>(i_num), temp) != extent_protocol::OK){
		ret = IOERR;
		goto release;
	}
release:
	unlock(i_num);
    return ret;

}

int yfs_client::read_dir(inum i_num, std::map<std::string, inum> &entries){
    int ret = yfs_client::OK;
    std::string content;
	lock(i_num);
    if (ec->get(static_cast<extent_protocol::extentid_t>(i_num), content) != extent_protocol::OK){
        ret = IOERR;
		goto release;
	}
    get_dir_entries(content, entries);
release:
	unlock(i_num);
    return ret;
}

int yfs_client::unlink(inum parent, std::string name){
	std::string content;
	std::map<std::string, inum> entries;
	std::map<std::string, inum>::iterator it;
	int ret = OK;
	lock(parent);
	if (ec->get(static_cast<extent_protocol::extentid_t>(parent), content) != extent_protocol::OK){
		ret = IOERR;
		goto release;
	}
	get_dir_entries(content, entries);
	if ((it = entries.find(name)) == entries.end()){
		ret = NOENT;
		goto release;
	}else{
		entries.erase(it);
		get_dir_content(entries, content);
		if (ec->put(static_cast<extent_protocol::extentid_t>(parent), content) != extent_protocol::OK){
			ret = IOERR;
			goto release;
		}
		ret = OK;
	}
release:
	unlock(parent);
	return ret;
}

int yfs_client::resize(inum i_num, size_t size){
    int ret = yfs_client::OK;
    std::string buf;
	lock(i_num);
    if (size == 0){
        if (ec->put(static_cast<extent_protocol::extentid_t>(i_num), "") != extent_protocol::OK){
            ret = IOERR;
			goto release;
        }
    }else{
		if (ec->get(static_cast<extent_protocol::extentid_t>(i_num), buf) != extent_protocol::OK){
			ret = IOERR;
			goto release;
		}
		if (buf.size() == size){
			ret = OK;
			goto release;
		}else{
			buf.resize(size, '\0');
			if (ec->put(static_cast<extent_protocol::extentid_t>(i_num), buf) != extent_protocol::OK){
				ret = IOERR;
				goto release;
			}
			ret = OK;
			goto release;
		}
	}

release:
	unlock(i_num);
    return ret;
}
