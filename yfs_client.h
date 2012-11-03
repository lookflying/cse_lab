#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include <map>

#include "lock_protocol.h"
#include "lock_client.h"

class yfs_client {
    extent_client *ec;
	lock_client *lc;
public:

    typedef unsigned long long inum;
    enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
    typedef int status;

    struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
    };
    struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
    };
    struct dirent {
    std::string name;
    yfs_client::inum inum;
    };

private:
    static std::string name(inum);
    static inum n2i(std::string);
    static const char delimiter = ':';
    static const char entry_delimiter = '\n';
    static std::string to_dir_entry(std::string name, inum i_num);
    static void to_name_inum(std::string dir_entry, std::string &name, inum &i_num);
    static void get_dir_entries(std::string content, std::map<std::string, inum> &entries);
    static void get_dir_content(std::map<std::string, inum> entries, std::string &content);

	void lock(inum id);
	void unlock(inum id);
public:

    yfs_client(std::string, std::string);
	~yfs_client();
    
	bool isfile(inum);
    bool isdir(inum);

    int getfile(inum, fileinfo &);
    int getdir(inum, dirinfo &);

    int look_up_by_name(inum parent, std::string name, inum &i_num);
    int create_file(inum parent, std::string name, inum &i_num);
    int read_file(inum i_num, std::string &buf, off_t off, size_t &size);
    int write_file(inum i_num, std::string buf, off_t off, size_t &size);
    int read_dir(inum i_num, std::map<std::string, inum> &entries);
    int resize(inum i_num, size_t size);
	int mkdir(inum parent, std::string name, inum &i_num);
	int unlink(inum parent, std::string name); 

};

#endif 
