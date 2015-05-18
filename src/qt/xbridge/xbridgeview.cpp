//******************************************************************************
//******************************************************************************

#include "xbridgeview.h"
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
const QString testFromCurrency("XC");
const QString testToCurrency("SWIFT");
const QString testFromAmount("0.001");
const QString testToAmount("0.002");

//******************************************************************************
//******************************************************************************
XBridgeView::XBridgeView(QWidget *parent)
    : QWidget(parent)
{
    setupUI();

    uiInterface.NotifyXBridgeExchangeWalletsReceived.connect(boost::bind(&XBridgeView::onWalletListReceived, this, _1));
}

//******************************************************************************
//******************************************************************************
XBridgeView::~XBridgeView()
{

}

//******************************************************************************
//******************************************************************************
void XBridgeView::setupUI()
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
    m_currencyFrom->setModel(&m_walletsModel);
    grid->addWidget(m_currencyFrom, 3, 1, 1, 1);

    m_amountTo = new QLineEdit(this);
    m_amountTo->setText(testToAmount);
    grid->addWidget(m_amountTo, 3, 3, 1, 1);

    m_currencyTo = new QComboBox(this);
    m_currencyTo->setModel(&m_walletsModel);
    grid->addWidget(m_currencyTo, 3, 4, 1, 1);

    l = new QLabel(trUtf8(" --- >>> "), this);
    grid->addWidget(l, 1, 2, 3, 1, Qt::AlignHCenter | Qt::AlignCenter);

    m_btnSend = new QPushButton(trUtf8("send transaction"), this);
    m_btnSend->setEnabled(false);

    QHBoxLayout * hbox = new QHBoxLayout;
    hbox->addStretch();
    hbox->addWidget(m_btnSend);

    grid->addLayout(hbox, 4, 0, 1, 5);

    QVBoxLayout * vbox = new QVBoxLayout;
    vbox->addLayout(grid);
    vbox->addStretch();

    setLayout(vbox);

    VERIFY(connect(m_btnSend, SIGNAL(clicked()), this, SLOT(onSendTransaction())));
}

//******************************************************************************
//******************************************************************************
void XBridgeView::onWalletListReceived(const std::vector<std::pair<std::string, std::string> > & wallets)
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
void XBridgeView::onWalletListReceivedHandler(const QStringList & wallets)
{
    m_wallets = wallets;
    m_walletsModel.setStringList(m_wallets);

    int idx = m_wallets.indexOf(testFromCurrency);
    m_currencyFrom->setCurrentIndex(idx);

    idx = m_wallets.indexOf(testToCurrency);
    m_currencyTo->setCurrentIndex(idx);

    m_btnSend->setEnabled(true);
}

//******************************************************************************
//******************************************************************************
void XBridgeView::onSendTransaction()
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

    // TODO check amount
    xbridge().sendXBridgeTransaction(from, fromCurrency, (boost::uint64_t)(fromAmount * 1000000),
                                     to,   toCurrency,   (boost::uint64_t)(toAmount * 1000000));

    // TODO fix to address
    // std::vector<unsigned char> from = DecodeBase64("2J3u4r9D+pNS6ZPNMYR1tgl+wVI=");
    // std::vector<unsigned char> to   = DecodeBase64("BWU9J85uL4242RnXXhZfRdA8p9s=");
    // xbridge().sendXBridgeTransaction(from, "XC", 10 * 1000000, to, "SWIFT", 100 * 1000000);
}
