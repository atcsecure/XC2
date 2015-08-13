//******************************************************************************
//******************************************************************************

#include "xbridgelowlevel.h"
#include "util.h"
#include "ui_interface.h"

// TODO remove
#include <QDebug>

//******************************************************************************
//******************************************************************************
XBridgeLowLevel::XBridgeLowLevel()
    : m_ip("127.0.0.1")
    , m_port(SERVER_LISTEN_PORT)
    , m_socket(m_io)
    , m_timer(m_io, boost::posix_time::seconds(TIMER_INTERVAL))
{
    std::string addr = GetArg("-xbridge-address", "");
    if (addr.size())
    {
        int pos = addr.find(':');
        if (pos == -1)
        {
            m_ip = addr;
        }
        else
        {
            m_ip = addr.substr(0, pos);
            addr = addr.substr(pos+1, addr.length()-pos-1);
            int port = atoi(addr);
            if (port)
            {
                m_port = port;
            }
        }
    }
}

//******************************************************************************
//******************************************************************************
void XBridgeLowLevel::run()
{
    m_timer.async_wait(boost::bind(&XBridgeLowLevel::onTimer, this));

    m_io.run();
}

//*****************************************************************************
//*****************************************************************************
void XBridgeLowLevel::disconnect()
{
    m_socket.close();
}

//*****************************************************************************
//*****************************************************************************
void XBridgeLowLevel::onTimer()
{
    if (!m_socket.is_open())
    {
        // connect to localhost
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(m_ip), m_port);

        boost::system::error_code error;
        m_socket.connect(ep, error);
        if (!error)
        {
            onConnected();
            doReadHeader(XBridgePacketPtr(new XBridgePacket));
        }
        else
        {
            disconnect();
        }

    }

    handleTimer();

    // next
    m_timer.expires_at(m_timer.expires_at() + boost::posix_time::seconds(TIMER_INTERVAL));
    m_timer.async_wait(boost::bind(&XBridgeLowLevel::onTimer, this));
}

//*****************************************************************************
//*****************************************************************************
void XBridgeLowLevel::doReadHeader(XBridgePacketPtr packet,
                                  const std::size_t offset)
{
    m_socket.async_read_some(
                boost::asio::buffer(packet->header()+offset,
                                    packet->headerSize-offset),
                boost::bind(&XBridgeLowLevel::onReadHeader,
                            this,
                            packet, offset,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

//*****************************************************************************
//*****************************************************************************
void XBridgeLowLevel::onReadHeader(XBridgePacketPtr packet,
                                  const std::size_t offset,
                                  const boost::system::error_code & error,
                                  std::size_t transferred)
{
    if (error)
    {
        qDebug() << "ERROR <" << error.value() << "> " << error.message().c_str();
        disconnect();
        return;
    }

    if (offset + transferred != packet->headerSize)
    {
        qDebug() << "partially read header, read " << transferred
                 << " of " << packet->headerSize << " bytes";

        doReadHeader(packet, offset + transferred);
        return;
    }

    packet->alloc();
    doReadBody(packet);
}

//*****************************************************************************
//*****************************************************************************
void XBridgeLowLevel::doReadBody(XBridgePacketPtr packet,
                const std::size_t offset)
{
    m_socket.async_read_some(
                boost::asio::buffer(packet->data()+offset,
                                    packet->size()-offset),
                boost::bind(&XBridgeLowLevel::onReadBody,
                            this,
                            packet, offset,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

//*****************************************************************************
//*****************************************************************************
void XBridgeLowLevel::onReadBody(XBridgePacketPtr packet,
                                const std::size_t offset,
                                const boost::system::error_code & error,
                                std::size_t transferred = 0)
{
    if (error)
    {
        qDebug() << "ERROR <" << error.value() << "> " << error.message().c_str();
        disconnect();
        return;
    }

    if (offset + transferred != packet->size())
    {
        qDebug() << "partially read packet, read " << transferred
                 << " of " << packet->size() << " bytes";

        doReadBody(packet, offset + transferred);
        return;
    }

    if (!decryptPacket(packet))
    {
        qDebug() << "packet decoding error " << __FUNCTION__;
        return;
    }

    if (!processPacket(packet))
    {
        qDebug() << "packet processing error " << __FUNCTION__;
    }

    doReadHeader(XBridgePacketPtr(new XBridgePacket));
}

//*****************************************************************************
//*****************************************************************************
bool XBridgeLowLevel::sendPacket(XBridgePacket & packet)
{
    boost::system::error_code error;
    m_socket.send(boost::asio::buffer(packet.header(), packet.allSize()), 0, error);
    if (error)
    {
        qDebug() << "packet send error <"
                 << error.value() << "> " << error.message().c_str()
                 << " " << __FUNCTION__;
        return false;
    }
    return true;
}

//*****************************************************************************
//*****************************************************************************
bool XBridgeLowLevel::encryptPacket(XBridgePacketPtr /*packet*/)
{
    // TODO implement this
    return true;
}

//*****************************************************************************
//*****************************************************************************
bool XBridgeLowLevel::decryptPacket(XBridgePacketPtr /*packet*/)
{
    // TODO implement this
    return true;
}

//*****************************************************************************
//*****************************************************************************
bool XBridgeLowLevel::processPacket(XBridgePacketPtr packet)
{
    XBridgeCommand c = packet->command();

    if (m_processors.count(c) == 0)
    {
        m_processors[xbcInvalid](packet);
        qDebug() << "incorrect command code <" << c << "> ";
        return false;
    }

    if (!m_processors[c](packet))
    {
        qDebug() << "packet processing error <" << c << "> ";
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeLowLevel::connect()
{
    // start internal handlers
    m_thread = boost::thread(&XBridgeLowLevel::run, this);

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeLowLevel::isConnected() const
{
    return m_socket.is_open();
}

//*****************************************************************************
//*****************************************************************************
std::string XBridgeLowLevel::xbridgeAddressAndPort() const
{
    std::ostringstream o;
    o << m_ip << ":" << m_port;
    return o.str();
}
