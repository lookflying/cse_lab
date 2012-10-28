// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

lock_client::lock_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() < 0) {
    printf("lock_client: call bind\n");
  }
}

lock_client::~lock_client(){;
}

int
lock_client::stat(lock_protocol::lockid_t lid)
{
  int r;
  lock_protocol::status ret = cl->call(lock_protocol::stat, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

lock_protocol::status
lock_client::acquire(lock_protocol::lockid_t lid)
{
    int r;
    lock_protocol::status ret;
    ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);
    int interval = 1;
    while (ret == lock_protocol::RETRY){
        usleep(interval *= 2);
        ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);
    }
    VERIFY (ret == lock_protocol::OK);
    return ret;

}

lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid)
{
    int r;
    lock_protocol::status ret;
    ret = cl->call(lock_protocol::release, cl->id(), lid, r);
    return ret;

}

