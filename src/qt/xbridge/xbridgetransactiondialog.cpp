//******************************************************************************
//******************************************************************************

#include "xbridgetransactiondialog.h"
#include "../ui_interface.h"
#include "../xbridgeconnector.h"
#include "../util/verify.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

const QString testFrom("2J3u4r9D+pNS6ZPNMYR1tgl+wVI=");
const QString testTo("BWU9J85uL4242RnXXhZfRdA8p9s=");
// const QString testFromCurrency("XC");
// const QString testToCurrency("SWIFT");
const QString testFromAmount("0.001");
const QString testToAmount("0.002");

//******************************************************************************
//******************************************************************************
XBridgeTransactionDialog::XBridgeTransactionDialog(XBridgeTransactionsModel & model,
                                                   QWidget *parent)
    : QDialog(parent)
    , m_model(model)
{
    setupUI();

    m_walletsNotifier =
            uiInterface.NotifyXBridgeExchangeWalletsReceived.connect(boost::bind(&XBridgeTransactionDialog::onWalletListReceived, this, _1));

    m_thisWallets << m_model.thisCurrency();
    m_thisWalletsModel.setStringList(m_thisWallets);

    m_currencyFrom->setCurrentIndex(0);

    onWalletListReceived(xbridge().wallets());
}

//******************************************************************************
//******************************************************************************
XBridgeTransactionDialog::~XBridgeTransactionDialog()
{
    m_walletsNotifier.disconnect();
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionDialog::setPendingId(const uint256 & id)
{
    m_pendingId = id;
    bool isPending = m_pendingId != uint256();

    m_amountFrom->setEnabled(!isPending);
    m_amountTo->setEnabled(!isPending);
    m_currencyFrom->setEnabled(!isPending);
    m_currencyTo->setEnabled(!isPending);
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionDialog::setFromAmount(double amount)
{
    m_amountFrom->setText(QString::number(amount));
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionDialog::setToAmount(double amount)
{
    m_amountTo->setText(QString::number(amount));
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionDialog::setToCurrency(const QString & currency)
{
    int idx = m_wallets.indexOf(currency);
    m_currencyTo->setCurrentIndex(idx);
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionDialog::setupUI()
{
    QGridLayout * grid = new QGridLayout;

    QLabel * l = new QLabel(QString::fromStdString(xbridge().xbridgeAddressAndPort()), this);
    grid->addWidget(l, 0, 0, 1, 5);

    l = new QLabel(trUtf8("from"), this);
    grid->addWidget(l, 1, 0, 1, 1);

    l = new QLabel(trUtf8("to"), this);
    grid->addWidget(l, 1, 4, 1, 1, Qt::AlignRight);

    m_addressFrom = new QLineEdit(this);
    m_addressFrom->setText(testFrom);
    grid->addWidget(m_addressFrom, 2, 0, 1, 2);

    m_addressTo = new QLineEdit(this);
    m_addressTo->setText(testTo);
    grid->addWidget(m_addressTo, 2, 3, 1, 2);

    m_amountFrom = new QLineEdit(this);
    m_amountFrom->setText(testFromAmount);
    grid->addWidget(m_amountFrom, 3, 0, 1, 1);

    m_currencyFrom = new QComboBox(this);
    m_currencyFrom->setModel(&m_thisWalletsModel);
    grid->addWidget(m_currencyFrom, 3, 1, 1, 1);

    m_amountTo = new QLineEdit(this);
    m_amountTo->setText(testToAmount);
    grid->addWidget(m_amountTo, 3, 3, 1, 1);

    m_currencyTo = new QComboBox(this);
    m_currencyTo->setModel(&m_walletsModel);
    grid->addWidget(m_currencyTo, 3, 4, 1, 1);

    l = new QLabel(trUtf8(" --- >>> "), this);
    grid->addWidget(l, 1, 2, 3, 1, Qt::AlignHCenter | Qt::AlignCenter);

    m_btnSend = new QPushButton(trUtf8("New Transaction"), this);
    m_btnSend->setEnabled(false);

    QPushButton * cancel = new QPushButton(trUtf8("Cancel"), this);

    QHBoxLayout * hbox = new QHBoxLayout;
    hbox->addStretch();
    hbox->addWidget(m_btnSend);
    hbox->addWidget(cancel);

    grid->addLayout(hbox, 4, 0, 1, 5);

    QVBoxLayout * vbox = new QVBoxLayout;
    vbox->addLayout(grid);
    vbox->addStretch();

    setLayout(vbox);

    VERIFY(connect(m_btnSend, SIGNAL(clicked()), this, SLOT(onSendTransaction())));
    VERIFY(connect(cancel,    SIGNAL(clicked()), this, SLOT(reject())));
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionDialog::onWalletListReceived(const std::vector<std::pair<std::string, std::string> > & wallets)
{
    QStringList list;
    for (std::vector<std::pair<std::string, std::string> >::const_iterator i = wallets.begin();
         i != wallets.end(); ++i)
    {
        list.push_back(QString::fromStdString(i->first));
    }

    QMetaObject::invokeMethod(this, "onWalletListReceivedHandler", Qt::QueuedConnection,
                              Q_ARG(QStringList, list));
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionDialog::onWalletListReceivedHandler(const QStringList & wallets)
{
    QString w = m_currencyTo->currentText();

    m_wallets = wallets;
    m_walletsModel.setStringList(m_wallets);

    int idx = m_wallets.indexOf(w);
    m_currencyTo->setCurrentIndex(idx);

    m_btnSend->setEnabled(true);
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionDialog::onSendTransaction()
{
    std::vector<unsigned char> from = DecodeBase64(m_addressFrom->text().toStdString().c_str());
    std::vector<unsigned char> to   = DecodeBase64(m_addressTo->text().toStdString().c_str());
    if (from.size() == 0 || to.size() == 0)
    {
        QMessageBox::warning(this, trUtf8("check parameters"), trUtf8("Invalid address"));
        return;
    }

    std::string fromCurrency        = m_currencyFrom->currentText().toStdString();
    std::string toCurrency          = m_currencyTo->currentText().toStdString();
    if (fromCurrency.size() == 0 || toCurrency.size() == 0)
    {
        QMessageBox::warning(this, trUtf8("check parameters"), trUtf8("Invalid currency"));
        return;
    }

    double fromAmount      = m_amountFrom->text().toDouble();
    double toAmount        = m_amountTo->text().toDouble();
    if (fromAmount == 0 || toAmount == 0)
    {
        QMessageBox::warning(this, trUtf8("check parameters"), trUtf8("Invalid amount"));
        return;
    }

    if (m_pendingId != uint256())
    {
        // accept pending tx
        m_model.newTransactionFromPending(m_pendingId, from, to);
    }
    else
    {
        // new tx
        m_model.newTransaction(from, to, fromCurrency, toCurrency, fromAmount, toAmount);
    }

    accept();
}
