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
	ec = new extent_client(extent_dst);
	std::string buf;
	extent_protocol::extentid_t rootid = 0x00000001;
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

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the directory lock

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

int yfs_client::look_up_by_name(inum parent, std::string name, inum &i_num){
    int ret;
    if (!isdir(parent) && parent != 0x000000001){
        i_num = 0;
        return NOENT;
    }
    std::string content;
    if ((ret = ec->get(parent, content)) != extent_protocol::OK)
            return IOERR;
    std::map<std::string, inum> entries;
    get_dir_entries(content, entries);
    if (entries.find(name) != entries.end()){
        i_num = entries[name];
        return OK;
    }else{
        i_num = 0;
        return NOENT;
    }
}

int yfs_client::create_file(inum parent, std::string name, inum &i_num){
    i_num = 0x80000000;
    int r = static_cast<int>(i_num);
    if (ec->put(i_num, std::string(), r) != extent_protocol::OK)
        return IOERR;
    i_num = static_cast<inum>(static_cast<unsigned int>(r));
    if (i_num == 0x80000000){
        return IOERR;
    }else{
        std::string buf;
        if (ec->get(parent, buf) != extent_protocol::OK){
            return NOENT;
        }
        buf += (to_dir_entry(name, i_num) + entry_delimiter);
        if (ec->put(parent, buf) != extent_protocol::OK){
            return IOERR;
        }
        return OK;
    }
}

int yfs_client::mkdir(inum parent, std::string name, inum &i_num){
	i_num = 0x00000000;
	int r = static_cast<int>(i_num);
	if (ec->put(i_num, std::string(), r) != extent_protocol::OK){
		return IOERR;
	}
	i_num = static_cast<inum>(static_cast<unsigned int>(r));
	if (i_num == 0x00000000){
		return IOERR;
	}else{
		std::string buf;
		if (ec->get(parent, buf) != extent_protocol::OK){
			return NOENT;
		}
		buf += (to_dir_entry(name, i_num) + entry_delimiter);
		if (ec->put(parent, buf) != extent_protocol::OK){
			return IOERR;
		}
	}
	return OK;
}

int yfs_client::read_file(inum i_num, std::string &buf, off_t off, size_t &size){
    int ret = yfs_client::OK;
    std::string temp;
    if (ec->get(i_num, temp) != extent_protocol::OK)
        return IOERR;
    buf = temp.substr(off, size);
    size = buf.size();
    return ret;
}

int yfs_client::write_file(inum i_num, std::string buf, off_t off, size_t &size){
    int ret = yfs_client::OK;
    std::string temp;
    if (ec->get(i_num, temp) != extent_protocol::OK)
        return IOERR;
    size_t had = temp.size();
    if (had < static_cast<size_t>(off)){
        temp += std::string(off - had, '\0');
        temp += buf;
    }else if( had > static_cast<size_t>(off) + size){
        temp.replace(off, size, buf);
    }else{
        temp = temp.substr(0, off) + buf;
    }
    if (ec->put(i_num, temp) != extent_protocol::OK)
        return IOERR;
    return ret;

}

int yfs_client::read_dir(inum i_num, std::map<std::string, inum> &entries){
    int ret = yfs_client::OK;
    std::string content;
    if (ec->get(i_num, content) != extent_protocol::OK)
        return IOERR;
    get_dir_entries(content, entries);
    return ret;
}

int yfs_client::unlink(inum parent, std::string name){
	std::string content;
	std::map<std::string, inum> entries;
	if (ec->get(parent, content) != extent_protocol::OK){
		return IOERR;
	}
	get_dir_entries(content, entries);
	std::map<std::string, inum>::iterator it;
	if ((it = entries.find(name)) == entries.end()){
		return NOENT;
	}else{
		entries.erase(it);
		get_dir_content(entries, content);
		if (ec->put(parent, content) != extent_protocol::OK)
			return IOERR;
		return OK;
	}

}

int yfs_client::resize(inum i_num, size_t size){
    int ret = yfs_client::OK;
    fileinfo fi;
    if (size == 0){
        if (ec->put(i_num, "") != extent_protocol::OK){
            return IOERR;
        }
    }else{
        if ((ret = getfile(i_num, fi) != extent_protocol::OK))
                return ret;
        if (fi.size == size){
            return ret;
        }else if (fi.size > size){
            std::string buf;
            size_t read = size;
            if ((ret = read_file(i_num, buf, 0, read)) != yfs_client::OK)
                return ret;
            if (read != size)
                return IOERR;
            if (ec->put(i_num, buf) != extent_protocol::OK)
                return IOERR;
            return ret;
        }else {
            std::string buf;
            size_t read = fi.size;
            if ((ret = read_file(i_num, buf, 0, read)) != yfs_client::OK)
                return ret;
            if (read != fi.size)
                return IOERR;
            buf += std::string(size - read, '\0');
            if (ec->put(i_num, buf) != extent_protocol::OK)
                return IOERR;
            return ret;
        }
    }
    return ret;
}
