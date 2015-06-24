//******************************************************************************
//******************************************************************************

#ifndef XBRIDGETRANSACTIONDIALOG_H
#define XBRIDGETRANSACTIONDIALOG_H

#include "xbridge/xbridgetransactionsmodel.h"

#include <QDialog>
#include <QStringList>
#include <QStringListModel>

#include <boost/signals2.hpp>

class QLineEdit;
class QComboBox;
class QPushButton;

//******************************************************************************
//******************************************************************************
class XBridgeTransactionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit XBridgeTransactionDialog(XBridgeTransactionsModel & model, QWidget *parent = 0);
    ~XBridgeTransactionDialog();

    void setFromAmount(double amount);
    void setToAmount(double amount);
    void setToCurrency(const QString & currency);

signals:

private:
    void setupUI();

    void onWalletListReceived(const std::vector<std::pair<std::string, std::string> > & wallets);

private slots:
    void onWalletListReceivedHandler(const QStringList & wallets);
    void onSendTransaction();

private:
    XBridgeTransactionsModel & m_model;

    QLineEdit    * m_addressFrom;
    QLineEdit    * m_addressTo;
    QLineEdit    * m_amountFrom;
    QLineEdit    * m_amountTo;
    QComboBox    * m_currencyFrom;
    QComboBox    * m_currencyTo;
    QPushButton  * m_btnSend;

    QStringList      m_thisWallets;
    QStringListModel m_thisWalletsModel;

    QStringList      m_wallets;
    QStringListModel m_walletsModel;

    boost::signals2::connection m_walletsNotifier;
};

#endif // XBRIDGETRANSACTIONDIALOG_H
