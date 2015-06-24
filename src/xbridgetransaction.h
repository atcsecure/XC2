#ifndef XBRIDGETRANSACTION_H
#define XBRIDGETRANSACTION_H

#include "xbridgetransactiondescr.h"
#include "xbridgepacket.h"
#include "uint256.h"
#include "main.h"
#include "wallet.h"

#include <vector>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

//******************************************************************************
//******************************************************************************
struct XBridgeTransaction : public XBridgeTransactionDescr
{
    // CTransaction               payTx;
    CWalletTx                  payTx;
    CTransaction               revTx;

    XBridgePacketPtr           packet;

    // TODO add transaction state for gui
};

typedef boost::shared_ptr<XBridgeTransaction> XBridgeTransactionPtr;

#endif // XBRIDGETRANSACTION_H

