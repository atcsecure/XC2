//******************************************************************************
//******************************************************************************

#ifndef XBRIDGEPACKET_H
#define XBRIDGEPACKET_H

#include <vector>
#include <deque>
#include <memory>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

//******************************************************************************
//******************************************************************************
enum XBridgeCommand
{
    xbcInvalid = 0,

    // client use this message for announce your addresses for network
    //
    // xbcAnnounceAddresses
    //     uint160 client address
    xbcAnnounceAddresses,

    // xchat message
    //
    // xbcXChatMessage
    //     uint160 destination address
    //     serialized p2p message from bitcoin network
    xbcXChatMessage,

    //      client1                  hub          client2
    // xbcTransaction          -->    |
    // xbcTransaction          -->    |
    // xbcTransaction          -->    |
    // xbcTransaction          -->    |     <-- xbcTransaction
    //                                |     --> xbcTransactionHold
    //                                |     <-- xbcTransactionHoldApply
    // xbcTransactionHold      <--    |
    // xbcTransactionHoldApply -->    |
    // xbcTransactionPay       <--    |     --> xbcTransactionPay
    // xbcTransactionPayApply  -->    |     <-- xbcTransactionPayApply
    //
    //
    //      hub wallet 1              |           hub wallet 2
    // xbcReceivedTransaction  -->    |     <-- xbcReceivedTransaction
    // xbcTransactionFinish    <--    |     --> xbcTransactionFinish


    // exchange transaction
    //
    // xbcTransaction
    //    uint256 client transaction id
    //    uint160 source address
    //    8 bytes source currency
    //    uint64 source amount
    //    uint160 destination address
    //    8 bytes destination currency
    //    uint64 destination amount
    xbcTransaction,
    //
    // xbcTransactionHold
    //    uint160 client address
    //    uint160 hub address
    //    uint256 client transaction id
    //    uint256 hub transaction id
    xbcTransactionHold,
    //
    // xbcTransactionHoldApply
    //    uint160 hub address
    //    uint256 hub transaction id
    xbcTransactionHoldApply,
    //
    // xbcTransactionPay
    //    uint160 client address
    //    uint160 hub address
    //    uint256 hub transaction id
    //    uint160 hub wallet address
    xbcTransactionPay,
    //
    // xbcTransactionPayApply
    //    uint160 hub address
    //    uint256 hub transaction id
    //    uint256 payment id (bitcoin transaction hash)
    xbcTransactionPayApply,
    //
    // xbcTransactionCommit
    //    uint160 hub wallet address
    //    uint160 hub address
    //    uint256 hub transaction id
    //    uint160 client address
    //    uint64 amount
    xbcTransactionCommit,
    //
    // xbcTransactionCommitApply
    //    uint160 hub address
    //    uint256 hub transaction id
    xbcTransactionCommitApply,
    //
    // xbcTransactionCancel
    //    uint160 hub address
    //    uint256 hub transaction id
    //
    xbcTransactionCancel,
    //
    // xbcTransactionFinished
    //    uint160 client address
    //    uint256 hub transaction id
    //
    xbcTransactionFinished,
    //
    // xbcTransactionDropped
    //    uint160 address
    //    uint256 hub transaction id
    //
    xbcTransactionDropped,

    // smart hub periodically send this message for invitations to trading
    // this message contains address of smart hub and
    // list of connected wallets (btc, xc, etc...)
    // broadcast message
    //
    // xbcExchangeWallets
    //     {wallet id (string)}|{wallet title (string)}|{wallet id (string)}|{wallet title (string)}
    xbcExchangeWallets,

    // wallet send transaction hash when transaction received
    //
    // xbcReceivedTransaction
    //     uint256 transaction id (bitcoin transaction hash)
    xbcReceivedTransaction
};

//******************************************************************************
//******************************************************************************
typedef boost::uint32_t crc_t;

//******************************************************************************
//******************************************************************************
class XBridgePacket
{
    std::vector<unsigned char> m_body;

public:
    enum
    {
        headerSize = 8,
        commandSize = sizeof(boost::uint32_t)
    };

    std::size_t     size()    const     { return sizeField(); }
    std::size_t     allSize() const     { return m_body.size(); }

    crc_t           crc()     const
    {
        // TODO implement this
        assert(false || "not implemented");
        return (0);
        // return crcField();
    }

    XBridgeCommand command() const      { return static_cast<XBridgeCommand>(commandField()); }

    void    alloc()             { m_body.resize(headerSize + size()); }

    unsigned char  * header()            { return &m_body[0]; }
    unsigned char  * data()              { return &m_body[headerSize]; }

    // boost::int32_t int32Data() const { return field32<2>(); }

    void    clear()
    {
        m_body.resize(headerSize);
        commandField() = 0;
        sizeField() = 0;

        // TODO crc
        // crcField() = 0;
    }

    void resize(const unsigned int size)
    {
        m_body.resize(size+headerSize);
        sizeField() = size;
    }

    void    setData(const unsigned char data)
    {
        m_body.resize(sizeof(data) + headerSize);
        sizeField() = sizeof(data);
        m_body[headerSize] = data;
    }

    void    setData(const boost::int32_t data)
    {
        m_body.resize(sizeof(data) + headerSize);
        sizeField() = sizeof(data);
        field32<2>() = data;
    }

    void    setData(const std::string & data)
    {
        m_body.resize(data.size() + headerSize);
        sizeField() = data.size();
        if (data.size())
        {
            data.copy((char *)(&m_body[headerSize]), data.size());
        }
    }

    void    setData(const std::vector<unsigned char> & data, const unsigned int offset = 0)
    {
        setData(&data[0], data.size(), offset);
    }

    void    setData(const unsigned char * data, const unsigned int size, const unsigned int offset = 0)
    {
        unsigned int off = offset + headerSize;
        if (size)
        {
            if (m_body.size() < size+off)
            {
                m_body.resize(size+off);
                sizeField() = size+off-headerSize;
            }
            memcpy(&m_body[off], data, size);
        }
    }

    void append(const boost::uint32_t data)
    {
        unsigned char * ptr = (unsigned char *)&data;
        std::copy(ptr, ptr+sizeof(data), std::back_inserter(m_body));
        sizeField() = m_body.size() - headerSize;
    }

    void append(const boost::uint64_t data)
    {
        unsigned char * ptr = (unsigned char *)&data;
        std::copy(ptr, ptr+sizeof(data), std::back_inserter(m_body));
        sizeField() = m_body.size() - headerSize;
    }

    void append(const unsigned char * data, const int size)
    {
        std::copy(data, data+size, std::back_inserter(m_body));
        sizeField() = m_body.size() - headerSize;
    }

    void append(const std::vector<unsigned char> & data)
    {
        std::copy(data.begin(), data.end(), std::back_inserter(m_body));
        sizeField() = m_body.size() - headerSize;
    }

    void    copyFrom(const std::vector<unsigned char> & data)
    {
        m_body = data;

        if (sizeField() != data.size()-headerSize)
        {
            assert(false || "incorrect data size in XBridgePacket::copyFrom");
        }

        // TODO check packet crc
    }

    XBridgePacket() : m_body(headerSize, 0)
    {}

    explicit XBridgePacket(const std::string& raw) : m_body(raw.begin(), raw.end())
    {}

    XBridgePacket(const XBridgePacket & other)
    {
        m_body = other.m_body;
    }

    XBridgePacket(XBridgeCommand c) : m_body(headerSize, 0)
    {
        commandField() = static_cast<boost::uint32_t>(c);
    }

    XBridgePacket & operator = (const XBridgePacket & other)
    {
        m_body    = other.m_body;

        return *this;
    }

private:
    template<std::size_t INDEX>
    boost::uint32_t &       field32()
        { return *static_cast<boost::uint32_t *>(static_cast<void *>(&m_body[INDEX * 4])); }

    template<std::size_t INDEX>
    boost::uint32_t const& field32() const
        { return *static_cast<boost::uint32_t const*>(static_cast<void const*>(&m_body[INDEX * 4])); }

    boost::uint32_t &       commandField()       { return field32<0>(); }
    boost::uint32_t const & commandField() const { return field32<0>(); }
    boost::uint32_t &       sizeField()          { return field32<1>(); }
    boost::uint32_t const & sizeField() const    { return field32<1>(); }

    // TODO implement this
//    boost::uint32_t &       crcField()           { return field32<0>(); }
//    boost::uint32_t const & crcField() const     { return field32<0>(); }
};

typedef boost::shared_ptr<XBridgePacket> XBridgePacketPtr;
typedef std::deque<XBridgePacketPtr>   XBridgePacketQueue;

#endif // XBRIDGEPACKET_H
