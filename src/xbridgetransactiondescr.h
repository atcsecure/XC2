#ifndef XBRIDGETRANSACTIONDESCR
#define XBRIDGETRANSACTIONDESCR

#include "uint256.h"

#include <string>
#include <boost/cstdint.hpp>

//******************************************************************************
//******************************************************************************
struct XBridgeTransactionDescr
{
    enum
    {
        COIN = 1000000
    };

    enum State
    {
        trNew = 0,
        trPending,
        trAccepting,
        trHold,
        trCreated,
        trSigned,
        trCommited,
        trFinished,
        trRollback,
        trDropped,
        trCancelled,
        trInvalid
    };

    uint256                    id;

    std::vector<unsigned char> from;
    std::string                fromCurrency;
    boost::uint64_t            fromAmount;
    std::vector<unsigned char> to;
    std::string                toCurrency;
    boost::uint64_t            toAmount;

    State                      state;

    XBridgeTransactionDescr() : state(trNew) {}
};

#endif // XBRIDGETRANSACTIONDESCR

