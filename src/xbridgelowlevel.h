//******************************************************************************
//******************************************************************************

#ifndef XBRIDGELOWLEVEL_H
#define XBRIDGELOWLEVEL_H

#include "xbridgepacket.h"

#include "message.h"
#include "FastDelegate.h"

#include <boost/asio.hpp>
#include <boost/thread.hpp>

//******************************************************************************
//******************************************************************************
class XBridgeLowLevel
{
    friend class XBridgeConnector;

private:
    XBridgeLowLevel();

    enum
    {
        SERVER_LISTEN_PORT = 30330,
        TIMER_INTERVAL = 60
    };

public:
    bool connect();
    bool isConnected() const;

    std::string xbridgeAddressAndPort() const;

private:
    void run();

    void disconnect();

    virtual void onConnected() = 0;
    void onTimer();
    virtual void handleTimer() = 0;

    void doReadHeader(XBridgePacketPtr packet,
                      const std::size_t offset = 0);
    void onReadHeader(XBridgePacketPtr packet,
                      const std::size_t offset,
                      const boost::system::error_code & error,
                      std::size_t transferred);

    void doReadBody(XBridgePacketPtr packet,
                    const std::size_t offset = 0);
    void onReadBody(XBridgePacketPtr packet,
                    const std::size_t offset,
                    const boost::system::error_code & error,
                    std::size_t transferred);

    bool sendPacket(XBridgePacket & packet);

    bool encryptPacket(XBridgePacketPtr packet);
    bool decryptPacket(XBridgePacketPtr packet);
    bool processPacket(XBridgePacketPtr packet);

private:
    std::string                  m_ip;
    boost::uint32_t              m_port;

    boost::asio::io_service      m_io;
    boost::asio::ip::tcp::socket m_socket;
    boost::thread                m_thread;
    boost::asio::deadline_timer  m_timer;

private:
    // packet processors
    typedef std::map<const int, fastdelegate::FastDelegate1<XBridgePacketPtr, bool> > PacketProcessorsMap;
    PacketProcessorsMap m_processors;
};

#endif // XBRIDGELOWLEVEL_H
