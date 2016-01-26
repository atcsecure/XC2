//******************************************************************************
//******************************************************************************

#ifndef XBRIDGECONNECTOR_H
#define XBRIDGECONNECTOR_H

#include "xbridgepacket.h"
#include "xbridgelowlevel.h"
#include "xbridgetransaction.h"
#include "wallet.h"

#include <boost/thread/mutex.hpp>


//******************************************************************************
//******************************************************************************
class XBridgeConnector : public XBridgeLowLevel
{
    friend XBridgeConnector & xbridge();

private:
    XBridgeConnector();

private:
    virtual void onConnected();
    virtual void handleTimer();

public:
    static std::string thisCurrency();

    bool announceLocalAddresses();
    bool sendXChatMessage(const Message & m);

    uint256 sendXBridgeTransaction(const std::vector<unsigned char> & from,
                                   const std::string & fromCurrency,
                                   const boost::uint64_t fromAmount,
                                   const std::vector<unsigned char> & to,
                                   const std::string & toCurrency,
                                   const boost::uint64_t toAmount);
    bool cancelXBridgeTransaction(const uint256 & id);
    bool revertXBridgeTransaction(const uint256 & id);

    bool transactionReceived(const uint256 & hash);

    std::vector<std::pair<std::string, std::string> > wallets() const;

    bool sendAddressBookEntry(const std::string & currency,
                              const std::string & name,
                              const std::string & address);

    bool haveTransactionForRollback(const uint256 & walletTxId);
    bool rollbackTransaction(const uint256 & walletTxId);

private:
    CScript destination(const std::vector<unsigned char> & address);

    bool sendPendingTransaction(XBridgeTransactionPtr & ptr);
    bool sendCancelTransaction(const uint256 & txid);

    std::string txToString(const CTransaction & tx) const;
    CTransaction txFromString(const std::string & str) const;

private:
    bool processInvalid(XBridgePacketPtr packet);
    bool processXChatMessage(XBridgePacketPtr packet);

    bool processExchangeWallets(XBridgePacketPtr packet);
    bool processPendingTransaction(XBridgePacketPtr packet);

    bool processTransactionHold(XBridgePacketPtr packet);
    bool processTransactionInit(XBridgePacketPtr packet);
    bool processTransactionCreate(XBridgePacketPtr packet);
    bool processTransactionSign(XBridgePacketPtr packet);
    bool processTransactionCommit(XBridgePacketPtr packet);
    // bool processTransactionConfirm(XBridgePacketPtr packet);
    bool processTransactionFinished(XBridgePacketPtr packet);
    bool processTransactionCancel(XBridgePacketPtr packet);
    bool processTransactionRollback(XBridgePacketPtr packet);
    bool processTransactionDropped(XBridgePacketPtr packet);

    bool processAddressBookEntry(XBridgePacketPtr packet);

private:
    boost::mutex                             m_txLocker;
    std::map<uint256, XBridgeTransactionPtr> m_pendingTransactions;
    std::map<uint256, XBridgeTransactionPtr> m_transactions;

    // map wallet tx id after commit to xbridge tx structure for search
    std::map<uint256, uint256>               m_mapWalletTxToXBridgeTx;

    std::vector<std::pair<std::string, std::string> > m_receivedWallets;

    std::set<uint256> m_receivedTransactions;
};

//******************************************************************************
//******************************************************************************
XBridgeConnector & xbridge();

#endif // XBRIDGECONNECTOR_H
