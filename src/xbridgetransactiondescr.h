#ifndef XBRIDGETRANSACTIONDESCR
#define XBRIDGETRANSACTIONDESCR

#include "uint256.h"

#include <string>
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

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
        trInvalid,
        trExpired
    };

    uint256                    id;

    std::vector<unsigned char> from;
    std::string                fromCurrency;
    boost::uint64_t            fromAmount;
    std::vector<unsigned char> to;
    std::string                toCurrency;
    boost::uint64_t            toAmount;

    State                      state;

    boost::posix_time::ptime   created;
    boost::posix_time::ptime   txtime;

    XBridgeTransactionDescr()
        : state(trNew)
        , created(boost::posix_time::second_clock::universal_time())
        , txtime(boost::posix_time::second_clock::universal_time())
    {}

//    bool operator == (const XBridgeTransactionDescr & d) const
//    {
//        return id == d.id;
//    }

    bool operator < (const XBridgeTransactionDescr & d) const
    {
        return created < d.created;
    }

    bool operator > (const XBridgeTransactionDescr & d) const
    {
        return created > d.created;
    }

    XBridgeTransactionDescr & operator = (const XBridgeTransactionDescr & d)
    {
        if (this == &d)
        {
            return *this;
        }

        copyFrom(d);

        return *this;
    }

    XBridgeTransactionDescr(const XBridgeTransactionDescr & d)
    {
        state   = trNew;
        created = boost::posix_time::second_clock::universal_time();
        txtime  = boost::posix_time::second_clock::universal_time();

        copyFrom(d);
    }

private:
    void copyFrom(const XBridgeTransactionDescr & d)
    {
        id           = d.id;
        from         = d.from;
        fromCurrency = d.fromCurrency;
        fromAmount   = d.fromAmount;
        to           = d.to;
        toCurrency   = d.toCurrency;
        toAmount     = d.toAmount;
        state        = d.state;
        txtime       = boost::posix_time::second_clock::universal_time();
        if (created > d.created)
        {
            created = d.created;
        }
    }
};

#endif // XBRIDGETRANSACTIONDESCR

