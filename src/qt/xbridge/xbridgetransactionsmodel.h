//******************************************************************************
//******************************************************************************

#ifndef XBRIDGETRANSACTIONSMODEL_H
#define XBRIDGETRANSACTIONSMODEL_H

#include "../../uint256.h"
#include "../../xbridgetransactiondescr.h"

#include <QAbstractTableModel>
#include <QStringList>
#include <QTimer>

#include <vector>
#include <string>
#include <boost/cstdint.hpp>

//******************************************************************************
//******************************************************************************
class XBridgeTransactionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    XBridgeTransactionsModel();
    ~XBridgeTransactionsModel();

    enum ColumnIndex
    {
        CreationDate = 0,
        FirstColumn  = CreationDate,
        AddressFrom  = 1,
        AmountFrom   = 2,
        AddressTo    = 3,
        AmountTo     = 4,
        State        = 5,
        LastColumn   = State
    };

public:
    static QString   thisCurrency();

    virtual int      rowCount(const QModelIndex &) const;
    virtual int      columnCount(const QModelIndex &) const;
    virtual QVariant data(const QModelIndex & idx, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    bool isMyTransaction(const unsigned int index) const;

    bool newTransaction(const std::vector<unsigned char> & from,
                        const std::vector<unsigned char> & to,
                        const std::string & fromCurrency,
                        const std::string & toCurrency,
                        const double fromAmount,
                        const double toAmount);
    bool newTransactionFromPending(const uint256 & id,
                                   const std::vector<unsigned char> & from,
                                   const std::vector<unsigned char> & to);

    bool cancelTransaction(const uint256 & id);

    XBridgeTransactionDescr item(const unsigned int index) const;

private slots:
    void onTimer();

private:
    void onTransactionReceived(const XBridgeTransactionDescr & tx);
    void onTransactionIdChanged(const uint256 & id, const uint256 & newid);
    void onTransactionStateChanged(const uint256 & id, const unsigned int state);

    QString transactionState(const XBridgeTransactionDescr::State state) const;

private:
    QStringList m_columns;

    std::vector<XBridgeTransactionDescr> m_transactions;

    QTimer m_timer;
};

#endif // XBRIDGETRANSACTIONSMODEL_H
