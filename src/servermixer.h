#pragma once
#include <map>
#include <iostream>
#include <string>

#include "util.h"
#include "sync.h"
#include "mixer.h"
#include "net.h"
#include "bitcoinrpc.h"
#include "main.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread.hpp>

using namespace json_spirit;

class CServerMixTx {
 public:
  int stage;
  double amount;
  std::string addrName;
  int usesendfrommixer;
  RSA* servrsa;

  CServerMixTx() {}
};

extern CCriticalSection cs_mapServerMixTxs;
extern std::map<int64, CServerMixTx> mapServerMixTxs;

void ServerMix(std::string iaddress, std::string target, double amount, bool multipath);
