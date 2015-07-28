//******************************************************************************
//******************************************************************************

#include "xbridgetransactionsmodel.h"
#include "xbridgeconnector.h"

#include "../ui_interface.h"
#include "../util/verify.h"

//******************************************************************************
//******************************************************************************
XBridgeTransactionsModel::XBridgeTransactionsModel()
{
    m_columns << trUtf8("From") << trUtf8("Amount")
              << trUtf8("To") << trUtf8("Amount")
              << trUtf8("State");

    uiInterface.NotifyXBridgePendingTransactionReceived.connect
            (boost::bind(&XBridgeTransactionsModel::onTransactionReceived, this, _1));

    uiInterface.NotifyXBridgeTransactionStateChanged.connect
            (boost::bind(&XBridgeTransactionsModel::onTransactionStateChanged, this, _1, _2));

    uiInterface.NotifyXBridgeTransactionIdChanged.connect
            (boost::bind(&XBridgeTransactionsModel::onTransactionIdChanged, this, _1, _2));

    VERIFY(connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimer())));
    m_timer.start(3000);
}

//******************************************************************************
//******************************************************************************
XBridgeTransactionsModel::~XBridgeTransactionsModel()
{

}

//******************************************************************************
//******************************************************************************
// static
QString XBridgeTransactionsModel::thisCurrency()
{
    return "XC";
}

//******************************************************************************
//******************************************************************************
int XBridgeTransactionsModel::rowCount(const QModelIndex &) const
{
    return m_transactions.size();
}

//******************************************************************************
//******************************************************************************
int XBridgeTransactionsModel::columnCount(const QModelIndex &) const
{
    return m_columns.size();
}

//******************************************************************************
//******************************************************************************
QVariant XBridgeTransactionsModel::data(const QModelIndex & idx, int role) const
{
    if (!idx.isValid())
    {
        return QVariant();
    }

    if (idx.row() < 0 || idx.row() >= m_transactions.size())
    {
        return QVariant();
    }

    const XBridgeTransactionDescr & d = m_transactions[idx.row()];

    if (role == Qt::DisplayRole)
    {
        switch (idx.column())
        {
            case AddressFrom:
            {
                QString text;
                if (d.from.size())
                {
                    text = QString::fromStdString(EncodeBase64(&d.from[0], d.from.size()));
                }
                return QVariant(text);
            }
            case AmountFrom:
            {
                double amount = (double)d.fromAmount / XBridgeTransactionDescr::COIN;
                QString text = QString("%1 %2").arg(QString::number(amount), QString::fromStdString(d.fromCurrency));
                return QVariant(text);
            }
            case AddressTo:
            {
                QString text;
                if (d.to.size())
                {
                    text = QString::fromStdString(EncodeBase64(&d.to[0], d.to.size()));
                }
                return QVariant(text);
            }
            case AmountTo:
            {
                double amount = (double)d.toAmount / XBridgeTransactionDescr::COIN;
                QString text = QString("%1 %2").arg(QString::number(amount), QString::fromStdString(d.toCurrency));
                return QVariant(text);
            }
            case State:
            {
                return QVariant(transactionState(d.state));
            }
            default:
                return QVariant();
        }
    }

    return QVariant();
}

//******************************************************************************
//******************************************************************************
QVariant XBridgeTransactionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if (role == Qt::DisplayRole)
        {
            return m_columns[section];
        }
    }
    return QVariant();
}

//******************************************************************************
//******************************************************************************
XBridgeTransactionDescr XBridgeTransactionsModel::item(const unsigned int index) const
{
    if (index >= m_transactions.size())
    {
        XBridgeTransactionDescr dummy;
        dummy.state = XBridgeTransactionDescr::trInvalid;
        return dummy;
    }

    return m_transactions[index];
}

//******************************************************************************
//******************************************************************************
bool XBridgeTransactionsModel::isMyTransaction(const unsigned int index) const
{
    if (index >= m_transactions.size())
    {
        return false;
    }

    return m_transactions[index].from.size();
}

//******************************************************************************
//******************************************************************************
bool XBridgeTransactionsModel::newTransaction(const std::vector<unsigned char> & from,
                                              const std::vector<unsigned char> & to,
                                              const std::string & fromCurrency,
                                              const std::string & toCurrency,
                                              const double fromAmount,
                                              const double toAmount)
{

    // TODO check amount
    uint256 id = xbridge().sendXBridgeTransaction
            (from, fromCurrency, (boost::uint64_t)(fromAmount * XBridgeTransactionDescr::COIN),
             to,   toCurrency,   (boost::uint64_t)(toAmount * XBridgeTransactionDescr::COIN));

    if (id != uint256())
    {
        XBridgeTransactionDescr d;
        d.id           = id;
        d.from         = from;
        d.to           = to;
        d.fromCurrency = fromCurrency;
        d.toCurrency   = toCurrency;
        d.fromAmount   = (boost::uint64_t)(fromAmount * XBridgeTransactionDescr::COIN);
        d.toAmount     = (boost::uint64_t)(toAmount * XBridgeTransactionDescr::COIN);
        d.txtime       = boost::posix_time::second_clock::universal_time();

        onTransactionReceived(d);
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeTransactionsModel::newTransactionFromPending(const uint256 & id,
                                                         const std::vector<unsigned char> & from,
                                                         const std::vector<unsigned char> & to)
{
    for (unsigned int i = 0; i < m_transactions.size(); ++i)
    {
        if (m_transactions[i].id == id)
        {
            // found
            XBridgeTransactionDescr & d = m_transactions[i];
            d.from  = from;
            d.to    = to;
            d.state = XBridgeTransactionDescr::trAccepting;
            std::swap(d.fromCurrency, d.toCurrency);
            std::swap(d.fromAmount, d.toAmount);

            emit dataChanged(index(i, FirstColumn), index(i, LastColumn));

            // send tx
            d.id = xbridge().sendXBridgeTransaction(d.from, d.fromCurrency, d.fromAmount,
                                                    d.to,   d.toCurrency,   d.toAmount);

            d.txtime = boost::posix_time::second_clock::universal_time();

            break;
        }
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool XBridgeTransactionsModel::cancelTransaction(const uint256 & id)
{
    if (xbridge().cancelXBridgeTransaction(id))
    {
        for (unsigned int i = 0; i < m_transactions.size(); ++i)
        {
            if (m_transactions[i].id == id)
            {
                // found
                m_transactions[i].state = XBridgeTransactionDescr::trCancelled;
                emit dataChanged(index(i, FirstColumn), index(i, LastColumn));
            }
        }
        return true;
    }

    return false;
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsModel::onTimer()
{
    for (unsigned int i = 0; i < m_transactions.size(); ++i)
    {
        boost::posix_time::time_duration td =
                boost::posix_time::second_clock::universal_time() -
                m_transactions[i].txtime;
        if (td.total_seconds() > 60)
        {
            m_transactions[i].state = XBridgeTransactionDescr::trExpired;
            emit dataChanged(index(i, FirstColumn), index(i, LastColumn));
        }
    }
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsModel::onTransactionReceived(const XBridgeTransactionDescr & tx)
{
    for (unsigned int i = 0; i < m_transactions.size(); ++i)
    {
        if (m_transactions[i].id == tx.id)
        {
            // found
            if (m_transactions[i].from.size() == 0)
            {
                m_transactions[i] = tx;
            }

            else if (m_transactions[i].state < tx.state)
            {
                m_transactions[i].state = tx.state;
            }
            emit dataChanged(index(i, FirstColumn), index(i, LastColumn));
            return;
        }
    }

    // skip tx with other currencies
    std::string tmp = thisCurrency().toStdString();
    if (tx.fromCurrency != tmp && tx.toCurrency != tmp)
    {
        return;
    }

    emit beginInsertRows(QModelIndex(), m_transactions.size(), m_transactions.size());
    m_transactions.push_back(tx);
    emit endInsertRows();
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsModel::onTransactionIdChanged(const uint256 & id,
                                                      const uint256 & newid)
{
    for (std::vector<XBridgeTransactionDescr>::iterator i = m_transactions.begin();
         i != m_transactions.end(); ++i)
    {
        if (i->id == id)
        {
            i->id = newid;
            break;
        }
    }
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsModel::onTransactionStateChanged(const uint256 & id,
                                                         const unsigned int state)
{
    for (unsigned int i = 0; i < m_transactions.size(); ++i)
    {
        if (m_transactions[i].id == id)
        {
            // found
            m_transactions[i].state = static_cast<XBridgeTransactionDescr::State>(state);
            emit dataChanged(index(i, FirstColumn), index(i, LastColumn));
            return;
        }
    }
}

//******************************************************************************
//******************************************************************************
QString XBridgeTransactionsModel::transactionState(const XBridgeTransactionDescr::State state) const
{
    switch (state)
    {
        case XBridgeTransactionDescr::trInvalid:   return trUtf8("Invalid");
        case XBridgeTransactionDescr::trNew:       return trUtf8("New");
        case XBridgeTransactionDescr::trPending:   return trUtf8("Pending");
        case XBridgeTransactionDescr::trAccepting: return trUtf8("Accepting");
        case XBridgeTransactionDescr::trHold:      return trUtf8("Hold");
        case XBridgeTransactionDescr::trCreated:   return trUtf8("Created");
        case XBridgeTransactionDescr::trSigned:    return trUtf8("Signed");
        case XBridgeTransactionDescr::trCommited:  return trUtf8("Commited");
        case XBridgeTransactionDescr::trFinished:  return trUtf8("Finished");
        case XBridgeTransactionDescr::trCancelled: return trUtf8("Cancelled");
        case XBridgeTransactionDescr::trRollback:  return trUtf8("Rolled Back");
        case XBridgeTransactionDescr::trDropped:   return trUtf8("Dropped");
        case XBridgeTransactionDescr::trExpired:   return trUtf8("Expired");
        default:                                   return trUtf8("Unknown");
    }
}
