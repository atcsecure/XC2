//******************************************************************************
//******************************************************************************

#include "xbridgetransactionsmodel.h"
#include "xbridgeconnector.h"

#include "../ui_interface.h"

//******************************************************************************
//******************************************************************************
XBridgeTransactionsModel::XBridgeTransactionsModel()
{
    m_columns << trUtf8("From") << trUtf8("Amount")
              << trUtf8("To") << trUtf8("Amount")
              << trUtf8("State");

    uiInterface.NotifyXBridgePendingTransactionReceived.connect
            (boost::bind(&XBridgeTransactionsModel::onTransactionReceived, this, _1));
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

        onTransactionReceived(d);
    }

    return true;
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
QString XBridgeTransactionsModel::transactionState(const XBridgeTransactionDescr::State state) const
{
    switch (state)
    {
        case XBridgeTransactionDescr::trInvalid: return trUtf8("Invalid");
        case XBridgeTransactionDescr::trNew:     return trUtf8("New");
        case XBridgeTransactionDescr::trPending: return trUtf8("Pending");
        default:                                 return trUtf8("Unknown");
    }
}
