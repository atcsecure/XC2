//******************************************************************************
//******************************************************************************

#ifndef XBRIDGEVIEW_H
#define XBRIDGEVIEW_H

#include <QWidget>
#include <QStringList>
#include <QStringListModel>

class QLineEdit;
class QComboBox;
class QPushButton;

//******************************************************************************
//******************************************************************************
class XBridgeView : public QWidget
{
    Q_OBJECT
public:
    explicit XBridgeView(QWidget *parent = 0);
    ~XBridgeView();

signals:

private:
    void setupUI();

    void onWalletListReceived(const std::vector<std::pair<std::string, std::string> > & wallets);

private slots:
    void onWalletListReceivedHandler(const QStringList & wallets);
    void onSendTransaction();

private:
    QLineEdit   * m_addressFrom;
    QLineEdit   * m_addressTo;
    QLineEdit   * m_amountFrom;
    QLineEdit   * m_amountTo;
    QComboBox   * m_currencyFrom;
    QComboBox   * m_currencyTo;
    QPushButton * m_btnSend;

    QStringList      m_wallets;
    QStringListModel m_walletsModel;
};

#endif // XBRIDGEVIEW_H
