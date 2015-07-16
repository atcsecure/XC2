//******************************************************************************
//******************************************************************************

#ifndef XBRIDGETRANSACTIONSMODEL_H
#define XBRIDGETRANSACTIONSMODEL_H

#include "../../uint256.h"
#include "../../xbridgetransactiondescr.h"

#include <QAbstractTableModel>
#include <QStringList>

#include <vector>
#include <string>
#include <boost/cstdint.hpp>

//******************************************************************************
//******************************************************************************
class XBridgeTransactionsModel : public QAbstractTableModel
{
public:
    XBridgeTransactionsModel();
    ~XBridgeTransactionsModel();

    enum ColumnIndex
    {
        AddressFrom = 0,
        FirstColumn = AddressFrom,
        AmountFrom  = 1,
        AddressTo   = 2,
        AmountTo    = 3,
        State       = 4,
        LastColumn  = State
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

    bool cancelTransaction(const uint256 & id);

    XBridgeTransactionDescr item(const unsigned int index) const;

private:
    void onTransactionReceived(const XBridgeTransactionDescr & tx);
    void onTransactionStateChanged(const uint256 & id, const unsigned int state);

    QString transactionState(const XBridgeTransactionDescr::State state) const;

private:
    QStringList m_columns;

    std::vector<XBridgeTransactionDescr> m_transactions;
};

#endif // XBRIDGETRANSACTIONSMODEL_H
