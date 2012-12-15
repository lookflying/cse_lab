// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "rpc.h"
#include <string>
#include <map>
class extent_client {
private:
	rpcc *cl;
	struct cache_state_t{
		bool ditry_;
	};
	std::map<extent_protocol::extentid_t, extent_protocol::attr> attr_cache_;
	std::map<extent_protocol::extentid_t, std::string> data_cache_;

public:
	extent_client(std::string dst);

	extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
	extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &a);
	extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
	extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf, int &rid);
	extent_protocol::status remove(extent_protocol::extentid_t eid);
};

#endif 

