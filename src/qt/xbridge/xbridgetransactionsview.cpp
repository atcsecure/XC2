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
#include <QMessageBox>

//******************************************************************************
//******************************************************************************
XBridgeTransactionsView::XBridgeTransactionsView(QWidget *parent)
    : QWidget(parent)
    , m_dlg(m_txModel)
{
    setupUi();
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
#if QT_VERSION <0x050000
    header->setResizeMode(XBridgeTransactionsModel::AddressFrom, QHeaderView::Stretch);
#else
    header->setSectionResizeMode(XBridgeTransactionsModel::AddressFrom, QHeaderView::Stretch);
#endif
    header->resizeSection(XBridgeTransactionsModel::AmountFrom,  80);
#if QT_VERSION <0x050000
    header->setResizeMode(XBridgeTransactionsModel::AddressTo, QHeaderView::Stretch);
#else
    header->setSectionResizeMode(XBridgeTransactionsModel::AddressTo, QHeaderView::Stretch);
#endif
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
QMenu * XBridgeTransactionsView::setupContextMenu(QModelIndex & index)
{
    QMenu * contextMenu = new QMenu();

    if (!m_txModel.isMyTransaction(index.row()))
    {
        QAction * acceptTransaction = new QAction(tr("&Accept transaction"), this);
        contextMenu->addAction(acceptTransaction);

        VERIFY(connect(acceptTransaction,   SIGNAL(triggered()),
                       this,                SLOT(onAcceptTransaction())));
    }
    else
    {
        QAction * cancelTransaction = new QAction(tr("&Cancel transaction"), this);
        contextMenu->addAction(cancelTransaction);

        VERIFY(connect(cancelTransaction,   SIGNAL(triggered()),
                       this,                SLOT(onCancelTransaction())));
    }

    if (false)
    {
        QAction * rollbackTransaction = new QAction(tr("&Rollback transaction"), this);
        contextMenu->addAction(rollbackTransaction);

        VERIFY(connect(rollbackTransaction, SIGNAL(triggered()),
                       this,                SLOT(onRollbackTransaction())));
    }

    return contextMenu;
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsView::onNewTransaction()
{
    m_dlg.setPendingId(uint256());
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
    if (d.state != XBridgeTransactionDescr::trPending)
    {
        return;
    }

    m_dlg.setPendingId(d.id);
    m_dlg.setFromAmount((double)d.toAmount / XBridgeTransactionDescr::COIN);
    m_dlg.setToAmount((double)d.fromAmount / XBridgeTransactionDescr::COIN);
    m_dlg.setToCurrency(QString::fromStdString(d.fromCurrency));
    m_dlg.show();
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsView::onCancelTransaction()
{
    if (!m_contextMenuIndex.isValid())
    {
        return;
    }

    if (QMessageBox::warning(this,
                             trUtf8("Cancel transaction"),
                             trUtf8("Are you sure?"),
                             QMessageBox::Yes | QMessageBox::Cancel,
                             QMessageBox::Cancel) != QMessageBox::Yes)
    {
        return;
    }

    if (!m_txModel.cancelTransaction(m_txModel.item(m_contextMenuIndex.row()).id))
    {
        QMessageBox::warning(this,
                             trUtf8("Cancel transaction"),
                             trUtf8("Error send cancel request"));
    }
}

//******************************************************************************
//******************************************************************************
void XBridgeTransactionsView::onRollbackTransaction()
{

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

    QMenu * contextMenu = setupContextMenu(m_contextMenuIndex);

    contextMenu->exec(QCursor::pos());
    contextMenu->deleteLater();
}
