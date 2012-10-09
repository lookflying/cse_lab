// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>

lock_client::lock_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() < 0) {
    printf("lock_client: call bind\n");
  }
  VERIFY(pthread_mutex_init(&acquire_mutex_, NULL) == 0);
  VERIFY(pthread_mutex_init(&release_mutex_, NULL) == 0);
}

lock_client::~lock_client(){
    VERIFY(pthread_mutex_destroy(&acquire_mutex_) == 0);
    VERIFY(pthread_mutex_destroy(&release_mutex_) == 0);
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
    pthread_mutex_lock(&acquire_mutex_);
    lock_protocol::status ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);
    pthread_mutex_unlock(&acquire_mutex_);
    VERIFY (ret == lock_protocol::OK);
    return r;

}

lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid)
{
    int r;
    pthread_mutex_lock(&release_mutex_);
    lock_protocol::status ret = cl->call(lock_protocol::release, cl->id(), lid, r);
    pthread_mutex_unlock(&release_mutex_);
    VERIFY (ret == lock_protocol::OK);
    return r;

}

