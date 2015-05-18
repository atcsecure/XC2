#ifndef XBRIDGETRANSACTION_H
#define XBRIDGETRANSACTION_H

#include "xbridgepacket.h"
#include "uint256.h"

#include <vector>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

//******************************************************************************
//******************************************************************************
struct XBridgeTransaction
{
    uint256 id;
    std::vector<unsigned char> from;
    std::string fromCurrency;
    boost::uint64_t fromAmount;
    std::vector<unsigned char> to;
    std::string toCurrency;
    boost::uint64_t toAmount;

    // TODO add transaction state for gui

    XBridgePacketPtr packet;
};

typedef boost::shared_ptr<XBridgeTransaction> XBridgeTransactionPtr;

#endif // XBRIDGETRANSACTION_H

