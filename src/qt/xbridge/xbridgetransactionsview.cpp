//******************************************************************************
//******************************************************************************

#include "xbridgetransactionsview.h"
#include "xbridgetransactiondialog.h"
#include "util/verify.h"

#include <QTableView>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

//******************************************************************************
//******************************************************************************
XBridgeTransactionsView::XBridgeTransactionsView(QWidget *parent)
    : QWidget(parent)
    , m_dlg(m_txModel)
{
    setupUi();
    setupContextMenu();
}

//******************************************************************************
//******************************************************************************
XBridgeTransactionsView::~XBridgeTransactionsView()
{

}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsView::setupUi()
{
    QVBoxLayout * vbox = new QVBoxLayout;

    m_transactionsList = new QTableView(this);
    m_transactionsList->setModel(&m_txModel);
    m_transactionsList->setContextMenuPolicy(Qt::CustomContextMenu);
    VERIFY(connect(m_transactionsList, SIGNAL(customContextMenuRequested(QPoint)),
                   this,               SLOT(onContextMenu(QPoint))));


    QHeaderView * header = m_transactionsList->horizontalHeader();
    header->setResizeMode(XBridgeTransactionsModel::AddressFrom, QHeaderView::Stretch);
    header->resizeSection(XBridgeTransactionsModel::AmountFrom,  80);
    header->setResizeMode(XBridgeTransactionsModel::AddressTo,   QHeaderView::Stretch);
    header->resizeSection(XBridgeTransactionsModel::AmountTo,    80);
    header->resizeSection(XBridgeTransactionsModel::State,       128);
    vbox->addWidget(m_transactionsList);

    QHBoxLayout * hbox = new QHBoxLayout;

    QPushButton * addTxBtn = new QPushButton(trUtf8("New Transaction"), this);
    // addTxBtn->setIcon(QIcon("qrc://"))
    VERIFY(connect(addTxBtn, SIGNAL(clicked()), this, SLOT(onNewTransaction())));
    hbox->addWidget(addTxBtn);

    hbox->addStretch();

    vbox->addLayout(hbox);

    setLayout(vbox);
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsView::setupContextMenu()
{
    m_contextMenu = new QMenu();

    QAction * acceptTransaction = new QAction(tr("&Accept transaction"), this);
    m_contextMenu->addAction(acceptTransaction);

    VERIFY(connect(acceptTransaction,   SIGNAL(triggered()),
                   this,                SLOT(onAcceptTransaction())));

}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsView::onNewTransaction()
{
    m_dlg.show();
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsView::onAcceptTransaction()
{
    if (!m_contextMenuIndex.isValid())
    {
        return;
    }

    if (m_txModel.isMyTransaction(m_contextMenuIndex.row()))
    {
        return;
    }

    XBridgeTransactionDescr d = m_txModel.item(m_contextMenuIndex.row());
    if (d.state == XBridgeTransactionDescr::trInvalid)
    {
        return;
    }

    m_dlg.setFromAmount((double)d.toAmount / XBridgeTransactionDescr::COIN);
    m_dlg.setToAmount((double)d.fromAmount / XBridgeTransactionDescr::COIN);
    m_dlg.setToCurrency(QString::fromStdString(d.fromCurrency));
    m_dlg.show();
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsView::onContextMenu(QPoint pt)
{
    m_contextMenuIndex = m_transactionsList->indexAt(pt);
    if (!m_contextMenuIndex.isValid())
    {
        return;
    }

    if (m_txModel.isMyTransaction(m_contextMenuIndex.row()))
    {
        return;
    }

    m_contextMenu->exec(QCursor::pos());
}
