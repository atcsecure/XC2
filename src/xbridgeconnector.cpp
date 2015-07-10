//******************************************************************************
//******************************************************************************

#include "xbridgeconnector.h"
#include "xbridgetransactiondescr.h"
#include "base58.h"
#include "init.h"
#include "ui_interface.h"

#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

// TODO remove this
#include <QtDebug>

//******************************************************************************
//******************************************************************************
std::vector<std::string> getLocalBitcoinAddresses();

//******************************************************************************
//******************************************************************************
XBridgeConnector & xbridge()
{
    static XBridgeConnector connector;
    return connector;
}

//******************************************************************************
//******************************************************************************
XBridgeConnector::XBridgeConnector()
    : XBridgeLowLevel()
{
    m_processors[xbcInvalid]            .bind(this, &XBridgeConnector::processInvalid);
    m_processors[xbcXChatMessage]       .bind(this, &XBridgeConnector::processXChatMessage);

    m_processors[xbcExchangeWallets]    .bind(this, &XBridgeConnector::processExchangeWallets);
    m_processors[xbcPendingTransaction] .bind(this, &XBridgeConnector::processPendingTransaction);

    // transactions
    m_processors[xbcTransactionHold]    .bind(this, &XBridgeConnector::processTransactionHold);
    m_processors[xbcTransactionInit]    .bind(this, &XBridgeConnector::processTransactionInit);
    m_processors[xbcTransactionCreate]  .bind(this, &XBridgeConnector::processTransactionCreate);
    m_processors[xbcTransactionSign]    .bind(this, &XBridgeConnector::processTransactionSign);
    m_processors[xbcTransactionCommit]  .bind(this, &XBridgeConnector::processTransactionCommit);
    m_processors[xbcTransactionRollback].bind(this, &XBridgeConnector::processTransactionRollback);
    // m_processors[xbcTransactionConfirm] .bind(this, &XBridgeConnector::processTransactionConfirm);
    m_processors[xbcTransactionFinished].bind(this, &XBridgeConnector::processTransactionFinished);
    m_processors[xbcTransactionDropped] .bind(this, &XBridgeConnector::processTransactionCancel);
    m_processors[xbcTransactionDropped] .bind(this, &XBridgeConnector::processTransactionDropped);
}

//*****************************************************************************
//*****************************************************************************
void XBridgeConnector::handleTimer()
{
    if (m_socket.is_open())
    {
        // send local addresses
        announceLocalAddresses();
    }

    if (m_socket.is_open())
    {
        if (m_txLocker.try_lock())
        {
            // send pending transactions
            for (std::map<uint256, XBridgeTransactionPtr>::iterator i = m_pendingTransactions.begin();
                 i != m_pendingTransactions.end(); ++i)
            {
                XBridgeTransactionPtr ptr = i->second;
                // if (!ptr->packet)
                {
                    ptr->packet.reset(new XBridgePacket(xbcTransaction));

                    // field length must be 8 bytes
                    std::vector<unsigned char> fc(8, 0);
                    std::copy(ptr->fromCurrency.begin(), ptr->fromCurrency.end(), fc.begin());

                    // field length must be 8 bytes
                    std::vector<unsigned char> tc(8, 0);
                    std::copy(ptr->toCurrency.begin(), ptr->toCurrency.end(), tc.begin());

                    // 20 bytes - id of transaction
                    // 2x
                    // 20 bytes - address
                    //  8 bytes - currency
                    //  4 bytes - amount
                    ptr->packet->append(ptr->id.begin(), 32);
                    ptr->packet->append(ptr->from);
                    ptr->packet->append(fc);
                    ptr->packet->append(ptr->fromAmount);
                    ptr->packet->append(ptr->to);
                    ptr->packet->append(tc);
                    ptr->packet->append(ptr->toAmount);
                }

                if (!sendPacket(*(ptr->packet)))
                {
                    qDebug() << "send transaction error " << __FUNCTION__;
                }
            }

            m_txLocker.unlock();
        }
    }

    if (m_socket.is_open())
    {
        // send received transactions
        std::set<uint256> tmp = m_receivedTransactions;
        m_receivedTransactions.clear();

        for (std::set<uint256>::iterator i = tmp.begin(); i != tmp.end(); ++i)
        {
            transactionReceived(*i);
        }
    }
}

//*****************************************************************************
//*****************************************************************************
bool XBridgeConnector::processInvalid(XBridgePacketPtr /*packet*/)
{
    qDebug() << "xbcInvalid command processed";
    return true;
}

//*****************************************************************************
//*****************************************************************************
bool XBridgeConnector::processXChatMessage(XBridgePacketPtr packet)
{
    // size must be > 20 bytes (160bit)
    if (packet->size() <= 20)
    {
        qDebug() << "invalid packet size for xbcXChatMessage " << __FUNCTION__;
        return false;
    }

    // skip 20 bytes dest address
    CDataStream stream((const char *)(packet->data()+20),
                       (const char *)(packet->data()+packet->size()));

    Message m;
    stream >> m;

    bool isForMe = false;
    if (!m.process(isForMe))
    {
        // TODO need relay?
        // relay, if message not for me
        // m.broadcast();
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::announceLocalAddresses()
{
    if (!isConnected())
    {
        return false;
    }

    std::vector<std::string> addresses = getLocalBitcoinAddresses();

    BOOST_FOREACH(const std::string & addr, addresses)
    {
        std::vector<unsigned char> tmp;
        DecodeBase58Check(addr, tmp);
        if (tmp.empty())
        {
            continue;
        }

        // size of tmp must be 21 byte
        XBridgePacket p(xbcAnnounceAddresses);
        p.setData(&tmp[1], tmp.size()-1);

        // TODO encryption
//        if (!encryptPacket(p))
//        {
//            // TODO logs or signal to gui
//            return false;
//        }

        if (!sendPacket(p))
        {
            qDebug() << "send address error " << __FUNCTION__;
        }
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::sendXChatMessage(const Message & m)
{
    CDataStream stream;
    stream << m;

    std::vector<unsigned char> addr;
    DecodeBase58Check(m.to, addr);
    if (addr.empty())
    {
        // incorrect address
        return false;
    }

    XBridgePacket p(xbcXChatMessage);

    // copy dest address
    assert(addr.size() ==  21 || "address length must be 20 bytes + 1");
    p.setData(&addr[1], addr.size()-1);

    // copy message
    std::vector<unsigned char> message;
    std::copy(stream.begin(), stream.end(), std::back_inserter(message));
    p.setData(message, 20);

    // TODO encryption
//        if (!encryptPacket(p))
//        {
//            // TODO logs or signal to gui
//            return false;
//        }

    if (!sendPacket(p))
    {
        qDebug() << "send xchat message error " << __FUNCTION__;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
uint256 XBridgeConnector::sendXBridgeTransaction(const std::vector<unsigned char> & from,
                                                 const std::string & fromCurrency,
                                                 const boost::uint64_t fromAmount,
                                                 const std::vector<unsigned char> & to,
                                                 const std::string & toCurrency,
                                                 const boost::uint64_t toAmount)
{
    if (fromCurrency.size() > 8 || toCurrency.size() > 8)
    {
        assert(false || "invalid currency");
        return uint256();
    }

    uint256 id = Hash(from.begin(), from.end(),
                      fromCurrency.begin(), fromCurrency.end(),
                      BEGIN(fromAmount), END(fromAmount),
                      to.begin(), to.end(),
                      toCurrency.begin(), toCurrency.end(),
                      BEGIN(toAmount), END(toAmount));

    XBridgeTransactionPtr ptr(new XBridgeTransaction);
    ptr->id           = id;
    ptr->from         = from;
    ptr->fromCurrency = fromCurrency;
    ptr->fromAmount   = fromAmount;
    ptr->to           = to;
    ptr->toCurrency   = toCurrency;
    ptr->toAmount     = toAmount;

    {
        boost::mutex::scoped_lock l(m_txLocker);
        m_pendingTransactions[id] = ptr;
    }

    return id;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::cancelXBridgeTransaction(const uint256 & id)
{
    {
        boost::mutex::scoped_lock l(m_txLocker);
        m_pendingTransactions.erase(id);
        if (m_transactions.count(id))
        {
            m_transactions[id]->state = XBridgeTransactionDescr::trCancelled;
        }
    }

    return sendCancelTransaction(id);
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::revertXBridgeTransaction(const uint256 & id)
{
    // TODO temporary implementation
    XBridgeTransactionPtr xtx;
    {
        boost::mutex::scoped_lock l(m_txLocker);

        // search tx
        for (std::map<uint256, XBridgeTransactionPtr>::iterator i = m_transactions.begin();
             i != m_transactions.end(); ++i)
        {
            if (i->second->id == id)
            {
                xtx = i->second;
                break;
            }
        }
    }

    if (!xtx)
    {
        return false;
    }

    // rollback, commit revert transaction
    if (!xtx->revTx.CheckTransaction())
    {
        return false;
    }

    CReserveKey rkeys(pwalletMain);
    CWalletTx wtx(pwalletMain, xtx->revTx);
    if (!pwalletMain->CommitTransaction(wtx, rkeys))
    {
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::transactionReceived(const uint256 & hash)
{
    if (!m_socket.is_open())
    {
        m_receivedTransactions.insert(hash);
        return true;
    }

    // packet for this xbridge client, without adress
    XBridgePacket p(xbcReceivedTransaction);
    p.append(hash.begin(), 32);

    if (!sendPacket(p))
    {
        qDebug() << "send transaction hash error " << __FUNCTION__;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
std::vector<std::pair<std::string, std::string> > XBridgeConnector::wallets() const
{
    return m_receivedWallets;
}

//******************************************************************************
//******************************************************************************
CScript XBridgeConnector::destination(const std::vector<unsigned char> & address)
{
    uint160 uikey(address);
    CKeyID key(uikey);

    CBitcoinAddress baddr;
    baddr.Set(key);

    CScript addr;
    addr.SetDestination(baddr.Get());
    return addr;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::sendCancelTransaction(const uint256 & txid)
{
    XBridgePacket reply(xbcTransactionCancel);
    reply.append(txid.begin(), 32);
    if (!sendPacket(reply))
    {
        qDebug() << "error sending transaction cancel packet "
                 << __FUNCTION__;
        return false;
    }

    // cancelled
    return true;
}

//******************************************************************************
//******************************************************************************
std::string XBridgeConnector::txToString(const CTransaction & tx) const
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << tx;
    return HexStr(ss.begin(), ss.end());
}

//******************************************************************************
//******************************************************************************
CTransaction XBridgeConnector::txFromString(const std::string & str) const
{
    std::vector<unsigned char> txdata = ParseHex(str);
    CDataStream stream(txdata, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;
    stream >> tx;
    return tx;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processExchangeWallets(XBridgePacketPtr packet)
{
    std::string packetData((char *)packet->data(), packet->size());
    std::vector<std::string> strs;
    boost::split(strs, packetData, boost::is_any_of("|"));

    if (strs.size() % 2)
    {
        qDebug() << "incorrect count of strings for xbcExchangeWallets" << __FUNCTION__;
        return false;
    }

    std::vector<std::pair<std::string, std::string> > wallets;

    for (std::vector<std::string>::iterator i = strs.begin(); i != strs.end(); ++i)
    {
        std::string wallet = *i;
        std::string title = *(++i);
        // qDebug() << "wallet " << wallet.c_str() << " " << title.c_str();

        wallets.push_back(std::make_pair(wallet, title));
    }

    if (wallets.size())
    {
        m_receivedWallets = wallets;
        uiInterface.NotifyXBridgeExchangeWalletsReceived(wallets);
    }
    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processPendingTransaction(XBridgePacketPtr packet)
{
    if (packet->size() != 64)
    {
        qDebug() << "incorrect packet size for xbcTransactionHold" << __FUNCTION__;
        return false;
    }

    XBridgeTransactionDescr d;
    d.id           = uint256(packet->data());
    d.fromCurrency = std::string(reinterpret_cast<const char *>(packet->data()+32));
    d.fromAmount   = *reinterpret_cast<boost::uint64_t *>(packet->data()+40);
    d.toCurrency   = std::string(reinterpret_cast<const char *>(packet->data()+48));
    d.toAmount     = *reinterpret_cast<boost::uint64_t *>(packet->data()+56);
    d.state        = XBridgeTransactionDescr::trPending;

    uiInterface.NotifyXBridgePendingTransactionReceived(d);

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processTransactionHold(XBridgePacketPtr packet)
{
    if (packet->size() != 104)
    {
        qDebug() << "incorrect packet size for xbcTransactionHold" << __FUNCTION__;
        return false;
    }

    // this addr
    std::vector<unsigned char> thisAddress(packet->data(), packet->data()+20);

    // smart hub addr
    std::vector<unsigned char> hubAddress(packet->data()+20, packet->data()+40);

    // read packet data
    uint256 id   (packet->data()+40);
    uint256 newid(packet->data()+72);

    {
        boost::mutex::scoped_lock l(m_txLocker);

        if (!m_pendingTransactions.count(id))
        {
            // wtf? unknown transaction
            // TODO log
            return false;
        }

        // remove from pending
        XBridgeTransactionPtr xtx = m_pendingTransactions[id];
        m_pendingTransactions.erase(id);

        // move to processing
        m_transactions[newid] = xtx;

        xtx->state = XBridgeTransactionDescr::trHold;
    }

    uiInterface.NotifyXBridgeTransactionStateChanged(id);

    // send hold apply
    XBridgePacket reply(xbcTransactionHoldApply);
    reply.append(hubAddress);
    reply.append(newid.begin(), 32);

    if (!sendPacket(reply))
    {
        qDebug() << "error sending transaction hold reply packet " << __FUNCTION__;
        return false;
    }
    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processTransactionInit(XBridgePacketPtr packet)
{
    if (packet->size() != 144)
    {
        qDebug() << "incorrect packet size for xbcTransactionInit" << __FUNCTION__;
        return false;
    }

    std::vector<unsigned char> thisAddress(packet->data(), packet->data()+20);
    std::vector<unsigned char> hubAddress(packet->data()+20, packet->data()+40);

    uint256 txid(packet->data()+40);

    std::vector<unsigned char> from(packet->data()+72, packet->data()+92);
    std::string                fromCurrency(reinterpret_cast<const char *>(packet->data()+92));
    boost::uint64_t            fromAmount(*reinterpret_cast<boost::uint64_t *>(packet->data()+100));

    std::vector<unsigned char> to(packet->data()+108, packet->data()+128);
    std::string                toCurrency(reinterpret_cast<const char *>(packet->data()+128));
    boost::uint64_t            toAmount(*reinterpret_cast<boost::uint64_t *>(packet->data()+136));

    // create transaction
    // without id (non this client transaction)
    XBridgeTransactionPtr ptr(new XBridgeTransaction);
    // ptr->id           = txid;
    ptr->from         = from;
    ptr->fromCurrency = fromCurrency;
    ptr->fromAmount   = fromAmount;
    ptr->to           = to;
    ptr->toCurrency   = toCurrency;
    ptr->toAmount     = toAmount;

    {
        boost::mutex::scoped_lock l(m_txLocker);
        m_transactions[txid] = ptr;
    }

    // send initialized
    XBridgePacket reply(xbcTransactionInitialized);
    reply.append(hubAddress);
    reply.append(thisAddress);
    reply.append(txid.begin(), 32);

    if (!sendPacket(reply))
    {
        qDebug() << "error sending transaction hold reply packet " << __FUNCTION__;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processTransactionCreate(XBridgePacketPtr packet)
{
    if (packet->size() != 96)
    {
        qDebug() << "incorrect packet size for xbcTransactionCreate" << __FUNCTION__;
        return false;
    }

    std::vector<unsigned char> thisAddress(packet->data(), packet->data()+20);
    std::vector<unsigned char> hubAddress(packet->data()+20, packet->data()+40);

    // transaction id
    uint256 id   (packet->data()+40);

    // destination address
    std::vector<unsigned char> destAddress(packet->data()+72, packet->data()+92);

    // lock time
    boost::uint32_t lockTime = *reinterpret_cast<boost::uint32_t *>(packet->data()+92);

    XBridgeTransactionPtr xtx;
    {
        boost::mutex::scoped_lock l(m_txLocker);

        if (!m_transactions.count(id))
        {
            // wtf? unknown transaction
            // TODO log
            return false;
        }

        xtx = m_transactions[id];
    }

    boost::uint64_t outAmount = COIN*(static_cast<double>(xtx->fromAmount)/XBridgeTransactionDescr::COIN) + MIN_TX_FEE;
    boost::uint64_t inAmount  = 0;

    std::vector<COutput> coins;
    std::vector<COutput> usedInTx;
    pwalletMain->AvailableCoins(coins, false);

    // find inputs
    BOOST_FOREACH(const COutput& out, coins)
    {
        if (out.nDepth < 2)
        {
            // hardcode minimum of 2 confirmations
            continue;
        }

        usedInTx.push_back(out);
        inAmount += out.tx->vout[out.i].nValue;

        // check amount
        if (inAmount >= outAmount)
        {
            break;
        }
    }

    // check amount
    if (inAmount < outAmount)
    {
        // no money, cancel transaction
        sendCancelTransaction(id);
        return false;
    }

    // create tx1, locked
    CTransaction tx1;

    // lock time
    {
        time_t local = GetAdjustedTime();
        tx1.nLockTime = local + lockTime;
    }

    // inputs
    BOOST_FOREACH(const COutput & out, usedInTx)
    {
        CTxIn in(COutPoint(out.tx->GetHash(), out.i), CScript(), 0);
        tx1.vin.push_back(in);
    }

    // outputs
    tx1.vout.push_back(CTxOut(outAmount-MIN_TX_FEE, destination(destAddress)));
    if (inAmount > outAmount)
    {
        // rest
        CPubKey key;
        if (!pwalletMain->GetKeyFromPool(key, false))
        {
            // error, cancel tx
            sendCancelTransaction(id);
            return false;
        }

        CScript script;
        script.SetDestination(key.GetID());

        tx1.vout.push_back(CTxOut(inAmount-outAmount, script));
    }

    // serialize
    std::string unsignedTx1 = txToString(tx1);

    // sign tx1
    int i = 0;
    BOOST_FOREACH(const COutput & out, usedInTx)
    {
        if (!SignSignature(*pwalletMain, *out.tx, tx1, i++))
        {
            // do not sign, cancel
            sendCancelTransaction(id);
            return false;
        }
    }

    CWalletTx wtx1(pwalletMain, tx1);
    xtx->payTx = wtx1;

    // create tx2, inputs
    CTransaction tx2;
    CTxIn in(COutPoint(tx1.GetHash(), 0));
    tx2.vin.push_back(in);

    // outputs
    {
        CPubKey key;
        if (!pwalletMain->GetKeyFromPool(key, false))
        {
            // error, cancel tx
            sendCancelTransaction(id);
            return false;
        }

        CScript script;
        script.SetDestination(key.GetID());

        tx2.vout.push_back(CTxOut(outAmount-2*MIN_TX_FEE, script));
    }

    // TODO lock time for tx2
//    {
//        time_t local = 0;
//        time(&local);
//        tx2.nLockTime = local + 5;
//    }

    // serialize
    std::string unsignedTx2 = txToString(tx2);

    // store
    xtx->revTx = tx2;

    xtx->state = XBridgeTransactionDescr::trCreated;
    uiInterface.NotifyXBridgeTransactionStateChanged(id);

    // send reply
    XBridgePacket reply(xbcTransactionCreated);
    reply.append(hubAddress);
    reply.append(thisAddress);
    reply.append(id.begin(), 32);
    reply.append(unsignedTx1);
    reply.append(unsignedTx2);

    if (!sendPacket(reply))
    {
        qDebug() << "error sending created transactions packet " << __FUNCTION__;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processTransactionSign(XBridgePacketPtr packet)
{
    if (packet->size() < 72)
    {
        qDebug() << "incorrect packet size for xbcTransactionSign" << __FUNCTION__;
        return false;
    }

    std::vector<unsigned char> thisAddress(packet->data(), packet->data()+20);

    size_t offset = 20;
    std::vector<unsigned char> hubAddress(packet->data()+offset, packet->data()+offset+20);
    offset += 20;

    uint256 txid(packet->data()+offset);
    offset += 32;

    std::string rawtxpay(reinterpret_cast<const char *>(packet->data()+offset));
    offset += rawtxpay.size()+1;

    std::string rawtxrev(reinterpret_cast<const char *>(packet->data()+offset));

    // check txid
    XBridgeTransactionPtr xtx;
    {
        boost::mutex::scoped_lock l(m_txLocker);

        if (!m_transactions.count(txid))
        {
            // wtf? unknown transaction
            // TODO log
            return false;
        }

        xtx = m_transactions[txid];
    }

    // unserialize
    CTransaction txpay = txFromString(rawtxpay);
    CTransaction txrev = txFromString(rawtxrev);

    // TODO check txpay, sign txrevert
    for (size_t i = 0; i < txrev.vin.size(); ++i)
    {
        if (!SignSignature(*pwalletMain, txpay, txrev, i))
        {
            // not signed, cancel tx
            sendCancelTransaction(txid);
            return false;
        }
    }

    xtx->state = XBridgeTransactionDescr::trSigned;
    uiInterface.NotifyXBridgeTransactionStateChanged(txid);

    // send reply
    XBridgePacket reply(xbcTransactionSigned);
    reply.append(hubAddress);
    reply.append(thisAddress);
    reply.append(txid.begin(), 32);
    reply.append(txToString(txrev));

    if (!sendPacket(reply))
    {
        qDebug() << "error sending created transactions packet " << __FUNCTION__;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processTransactionCommit(XBridgePacketPtr packet)
{
    if (packet->size() < 72)
    {
        qDebug() << "incorrect packet size for xbcTransactionCommit" << __FUNCTION__;
        return false;
    }

    std::vector<unsigned char> thisAddress(packet->data(), packet->data()+20);
    std::vector<unsigned char> hubAddress(packet->data()+20, packet->data()+40);

    uint256 txid(packet->data()+40);

    std::string rawtx(reinterpret_cast<const char *>(packet->data()+72));

    // send pay transaction to network
    XBridgeTransactionPtr xtx;
    {
        boost::mutex::scoped_lock l(m_txLocker);

        if (!m_transactions.count(txid))
        {
            // wtf? unknown transaction
            // TODO log
            return false;
        }

        xtx = m_transactions[txid];
    }

    // unserialize signed transaction
    CTransaction txrev = txFromString(rawtx);
    xtx->revTx = txrev;

    CReserveKey rkeys(pwalletMain);
    if (!pwalletMain->CommitTransaction(xtx->payTx, rkeys))
    {
        // not commited....send cancel???
        // sendCancelTransaction(id);
        return false;
    }

    // xtx->payTx.GetHash();

    xtx->state = XBridgeTransactionDescr::trCommited;
    uiInterface.NotifyXBridgeTransactionStateChanged(txid);

    // send commit apply to hub
    XBridgePacket reply(xbcTransactionCommited);
    reply.append(hubAddress);
    reply.append(thisAddress);
    reply.append(txid.begin(), 32);
    reply.append((static_cast<CTransaction *>(&xtx->payTx))->GetHash().begin(), 32);
    if (!sendPacket(reply))
    {
        qDebug() << "error sending transaction commited packet "
                 << __FUNCTION__;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
//bool XBridgeConnector::processTransactionConfirm(XBridgePacketPtr packet)
//{
//    if (packet->size() < 72)
//    {
//        qDebug() << "incorrect packet size for xbcTransactionConfirm" << __FUNCTION__;
//        return false;
//    }

//    std::vector<unsigned char> thisAddress(packet->data(), packet->data()+20);
//    std::vector<unsigned char> hubAddress(packet->data()+20, packet->data()+40);

//    uint256 txid(packet->data()+40);
//    uint256 txhash(packet->data()+72);

//    return true;
//}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processTransactionFinished(XBridgePacketPtr packet)
{
    if (packet->size() != 52)
    {
        qDebug() << "incorrect packet size for xbcTransactionFinished" << __FUNCTION__;
        return false;
    }

    // transaction id
    uint256 txid(packet->data()+20);

    XBridgeTransactionPtr xtx;
    {
        boost::mutex::scoped_lock l(m_txLocker);

        if (!m_transactions.count(txid))
        {
            // wtf? unknown transaction
            // TODO log
            return false;
        }

        xtx = m_transactions[txid];
    }

    // update transaction state for gui
    xtx->state = XBridgeTransactionDescr::trFinished;
    uiInterface.NotifyXBridgeTransactionStateChanged(txid);

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processTransactionCancel(XBridgePacketPtr packet)
{
    if (packet->size() != 52)
    {
        qDebug() << "incorrect packet size for xbcTransactionDropped" << __FUNCTION__;
        return false;
    }

    // transaction id
    uint256 id(packet->data()+20);

    XBridgeTransactionPtr xtx;
    {
        boost::mutex::scoped_lock l(m_txLocker);

        if (!m_transactions.count(id))
        {
            // wtf? unknown transaction
            // TODO log
            return false;
        }

        xtx = m_transactions[id];
    }

    // update transaction state for gui
    xtx->state = XBridgeTransactionDescr::trCancelled;
    uiInterface.NotifyXBridgeTransactionStateChanged(id);

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processTransactionRollback(XBridgePacketPtr packet)
{
    if (packet->size() != 52)
    {
        qDebug() << "incorrect packet size for xbcTransactionRollback" << __FUNCTION__;
        return false;
    }

    // transaction id
    uint256 txid(packet->data()+20);

    // for rollback need local transaction id
    // TODO maybe hub id?
    XBridgeTransactionPtr xtx;
    {
        boost::mutex::scoped_lock l(m_txLocker);

        if (!m_transactions.count(txid))
        {
            // wtf? unknown tx
            // TODO log
            return false;
        }

        xtx = m_transactions[txid];
    }

    revertXBridgeTransaction(xtx->id);

    // update transaction state for gui
    xtx->state = XBridgeTransactionDescr::trRollback;
    uiInterface.NotifyXBridgeTransactionStateChanged(txid);

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeConnector::processTransactionDropped(XBridgePacketPtr packet)
{
    if (packet->size() != 52)
    {
        qDebug() << "incorrect packet size for xbcTransactionDropped" << __FUNCTION__;
        return false;
    }

    // transaction id
    uint256 id(packet->data()+20);

    XBridgeTransactionPtr xtx;
    {
        boost::mutex::scoped_lock l(m_txLocker);

        if (!m_transactions.count(id))
        {
            // wtf? unknown transaction
            // TODO log
            return false;
        }

        xtx = m_transactions[id];
    }

    // update transaction state for gui
    xtx->state = XBridgeTransactionDescr::trDropped;
    uiInterface.NotifyXBridgeTransactionStateChanged(id);

    return true;
}
