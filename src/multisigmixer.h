//*****************************************************************************
//*****************************************************************************

#pragma once

#include "util.h"
#include "sync.h"
#include "bitcoinrpc.h"
#include "wallet.h"
#include "script.h"

#include <iostream>                                                              
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

//*****************************************************************************
//*****************************************************************************
using namespace boost::posix_time;
using namespace boost::fusion;
using namespace boost;
using namespace std;
using namespace json_spirit;

//******************************************************`***********************
class CMultisigTx;
extern CMultisigTx    * msgtx;
extern CCriticalSection cs_msgtx;

//*****************************************************************************
// the transaction information
//*****************************************************************************
class CMultisigTx
{
public:
    enum
    {
        protocolVersion    = 1,

        baseTimeout        = 60,
        networkTimeout     = baseTimeout,
        dropSessionTimeout = baseTimeout+20
    };

public:
    bool           root;
    bool           joined;
    bool           joining;

    // previous node in the chain
    CNode        * prev;
    // next node in the chain
    CNode * next;

    vector<pair<string, int> >            inputs;
    vector<pair<string, boost::int64_t> > outputs;

    vector<pair<string, int> >            myinputs;
    vector<pair<string, boost::int64_t> > myoutputs;

    boost::int64_t amount;
    boost::int64_t fee;

    std::vector<std::string> autonodes;

    // the time the first node started searching
    ptime          start;

    static boost::uint32_t version() { return protocolVersion; }
};

//*****************************************************************************
// tell everyone we're trying to start a transaction of n coins with an existing
// m users
//*****************************************************************************
class CMultisigSearch
{
public:
    boost::uint32_t version;
    boost::int64_t  amount;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(amount);
    )

    CMultisigSearch()
    {
        version = 0;
        amount  = 0;
    }
};

//*****************************************************************************
// tell the broadcaster that we want to join the chain
//*****************************************************************************
class CMultisigJoin
{
public:
    boost::uint32_t version;
    boost::int64_t  amount;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(amount);
    )

    CMultisigJoin()
    {
        version = 0;
        amount  = 0;
    }
};

//*****************************************************************************
// broadcaster tells the node that it is now the next in the chain, do searches
//*****************************************************************************
class CMultisigAccept
{
public:
    boost::uint32_t version;
    boost::int64_t  amount;
    boost::int64_t  fee;
    string          start;

    std::vector<std::string> autonodes;

    std::vector<pair<string, boost::int64_t> > outputs;
    std::vector<pair<string, int> >            inputs;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(amount);
        READWRITE(start);
        READWRITE(inputs);
        READWRITE(outputs);
        READWRITE(fee);
        READWRITE(autonodes);
    )

    CMultisigAccept()
    {
        version = 0;
        amount  = 0;
        fee     = 0;
    }
};

//*****************************************************************************
// tell the next node in the chain to sign the tx and pass it on
//*****************************************************************************
class CMultisigSign
{
public:
    boost::uint32_t version;
    boost::int64_t  amount;
    std::string     rawtx;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(amount);
        READWRITE(rawtx);
    )

    CMultisigSign()
    {
        version = 0;
        amount  = 0;
    }
};

//*****************************************************************************
//*****************************************************************************
extern void start_distmix_search(boost::int64_t amount, ptime start);
extern void wait_for_distmix_search();
