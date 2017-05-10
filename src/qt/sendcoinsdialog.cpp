//*****************************************************************************
//*****************************************************************************

#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"
#include "init.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "addressbookpage.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "guiutil.h"
#include "askpassphrasedialog.h"
#include "coincontrol.h"
#include "coincontroldialog.h"
#include "bitcoinrpc.h"

#include "json/json_spirit.h"

#include <QMessageBox>
#include <QLocale>
#include <QTextDocument>
#include <QScrollBar>
#include <QClipboard>
#include <QFileDialog>
#include <QFile>
#include <QImageReader>
#include <QImage>
#include <QDebug>
#include <QMimeDatabase>
#include <QMimeType>

#include <boost/lexical_cast.hpp>

using namespace json_spirit;

//*****************************************************************************
//*****************************************************************************
SendCoinsDialog::SendCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog),
    model(0)
{
    ui->setupUi(this);

    on_txWithImage_clicked();

    // disable "secure transactions" when distmix autonode enabled
    if (GetBoolArg("-distmix-autonode"))
    {
        ui->checkDistmix->setEnabled(false);
    }

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif

#if QT_VERSION >= 0x040700
     /* Do not move this to the XML file, Qt before 4.7 will choke on it */
     ui->lineEditCoinControlChange->setPlaceholderText(tr("Enter a XCurrency address (e.g. BrXW1RKLDe8VMNwTwLwSiKuATN5M74EL85)"));
#endif

    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // Coin Control
    ui->lineEditCoinControlChange->setFont(GUIUtil::bitcoinAddressFont());
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->lineEditCoinControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(coinControlChangeEdited(const QString &)));

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy low output"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    m_isNewRecipientAllowed = true;
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setModel(model);
        }
    }
    if(model && model->getOptionsModel())
    {
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
		
        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        connect(model->getOptionsModel(), SIGNAL(transactionFeeChanged(qint64)), this, SLOT(coinControlUpdateLabels()));
        ui->frameCoinControl->setVisible(model->getOptionsModel()->getCoinControlFeatures());
        coinControlUpdateLabels();
    }
}

//*****************************************************************************
//*****************************************************************************
SendCoinsDialog::~SendCoinsDialog()
{
    delete ui;
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::on_sendButton_clicked()
{
    bool valid = true;

    // clear old payments
    m_sendEntries.clear();

    if (!model)
    {
        return;
    }
	
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
                m_sendEntries.push_back(entry);
            }
            else
            {
                valid = false;
            }
        }
    }

    if (!valid || !m_sendEntries.size())
    {
        m_sendEntries.clear();
        return;
    }

    // Format confirmation message
    QStringList formatted;
    foreach (SendCoinsEntry * entry, m_sendEntries)
    {
        SendCoinsRecipient rcp = entry->getValue();
#if QT_VERSION < 0x050000
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, rcp.amount), Qt::escape(rcp.label), rcp.address));
#else
         formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, rcp.amount), rcp.label.toHtmlEscaped(), rcp.address));
#endif
    }

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send coins"),
                          tr("Are you sure you want to send %1?").arg(formatted.join(tr(" and "))),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if (retval != QMessageBox::Yes)
    {
        m_sendEntries.clear();
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if (!ctx.isValid())
    {
        // Unlock wallet was cancelled
        m_sendEntries.clear();
        return;
    }

    WalletModel::SendCoinsReturn sendstatus;

    // check distmix checkbox
    if (ui->checkDistmix->isChecked())
    {
        if (!GetBoolArg("-distmix-autonode"))
        {
            setEnabledForProcessing(false);

            processDistmixPayments();
        }
        else
        {
            QMessageBox::warning(this, tr("Send Coins"),
                tr("Remove --distmix-autonode for use secure transactions."),
                QMessageBox::Ok, QMessageBox::Ok);
            ui->checkDistmix->setChecked(false);
            ui->checkDistmix->setEnabled(false);
        }
        return;
    }
    else if (ui->txWithImage->isChecked())
    {
        setEnabledForProcessing(false);

        processPaymentsWithImage();

        return;
    }

    // no checked distmix

    QList<SendCoinsRecipient> recipients;
    foreach (SendCoinsEntry * entry, m_sendEntries)
    {
        recipients.push_back(entry->getValue());
    }

    if (!model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures())
    {
        sendstatus = model->sendCoins(recipients);
    }
    else
    {
        sendstatus = model->sendCoins(recipients, CoinControlDialog::coinControl);
    }

    switch(sendstatus.status)
    {
        case WalletModel::InvalidAddress:
            QMessageBox::warning(this, tr("Send Coins"),
                tr("The recipient address is not valid, please recheck."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::InvalidAmount:
            QMessageBox::warning(this, tr("Send Coins"),
                tr("The amount to pay must be larger than 0."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::AmountExceedsBalance:
            QMessageBox::warning(this, tr("Send Coins"),
                tr("The amount exceeds your balance."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            QMessageBox::warning(this, tr("Send Coins"),
                tr("The total exceeds your balance when the %1 transaction fee is included.").
                arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, sendstatus.fee)),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::DuplicateAddress:
            QMessageBox::warning(this, tr("Send Coins"),
                tr("Duplicate address found, can only send to each address once per send operation."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::TransactionCreationFailed:
            QMessageBox::warning(this, tr("Send Coins"),
                tr("Error: Transaction creation failed."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::TransactionCommitFailed:
            QMessageBox::warning(this, tr("Send Coins"),
                tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::Aborted: // User aborted, nothing to do
            break;
        case WalletModel::OK:
            accept();
            CoinControlDialog::coinControl->UnSelectAll();
            coinControlUpdateLabels();
            break;
    }

    m_sendEntries.clear();
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        delete ui->entries->takeAt(0)->widget();
    }
    addEntry();

    updateRemoveEnabled();

    ui->sendButton->setDefault(true);
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::reject()
{
    clear();
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::accept()
{
    clear();
}

//*****************************************************************************
//*****************************************************************************
SendCoinsEntry *SendCoinsDialog::addEntry()
{
    SendCoinsEntry *entry = new SendCoinsEntry(this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendCoinsEntry*)), this, SLOT(removeEntry(SendCoinsEntry*)));
	connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));

    updateRemoveEnabled();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    QCoreApplication::instance()->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::updateRemoveEnabled()
{
    // Remove buttons are enabled as soon as there is more than one send-entry
    bool enabled = (ui->entries->count() > 1);
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setRemoveEnabled(enabled);
        }
    }
    setupTabChain(0);
	coinControlUpdateLabels();
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::removeEntry(SendCoinsEntry* entry)
{
    delete entry;
    updateRemoveEnabled();
}

//*****************************************************************************
//*****************************************************************************
QWidget *SendCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->addButton);
    QWidget::setTabOrder(ui->addButton, ui->sendButton);
    return ui->sendButton;
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::pasteEntry(const SendCoinsRecipient &rv)
{
    if (!m_isNewRecipientAllowed)
    {
        return;
    }

    SendCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendCoinsEntry *first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setValue(rv);
}

//*****************************************************************************
//*****************************************************************************
bool SendCoinsDialog::handleURI(const QString &uri)
{
    SendCoinsRecipient rv;
    // URI has to be valid
    if (GUIUtil::parseBitcoinURI(uri, &rv))
    {
        CBitcoinAddress address(rv.address.toStdString());
        if (!address.IsValid())
            return false;
        pasteEntry(rv);
        return true;
    }

    return false;
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    Q_UNUSED(stake);
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    if(!model || !model->getOptionsModel())
        return;

    int unit = model->getOptionsModel()->getDisplayUnit();
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update labelBalance with the current balance and the current unit
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), model->getBalance()));
    }
}

//*****************************************************************************
// Coin Control: copy label "Quantity" to clipboard
//*****************************************************************************
void SendCoinsDialog::coinControlClipboardQuantity()
{
     QApplication::clipboard()->setText(ui->labelCoinControlQuantity->text());
 }
 
//*****************************************************************************
// Coin Control: copy label "Amount" to clipboard
//*****************************************************************************
void SendCoinsDialog::coinControlClipboardAmount()
{
     QApplication::clipboard()->setText(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
 }
 
//*****************************************************************************
// Coin Control: copy label "Fee" to clipboard
//*****************************************************************************
void SendCoinsDialog::coinControlClipboardFee()
{
     QApplication::clipboard()->setText(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")));
 }
 
//*****************************************************************************
// Coin Control: copy label "After fee" to clipboard
//*****************************************************************************
void SendCoinsDialog::coinControlClipboardAfterFee()
{
     QApplication::clipboard()->setText(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")));
 }
 
//*****************************************************************************
// Coin Control: copy label "Bytes" to clipboard
//*****************************************************************************
void SendCoinsDialog::coinControlClipboardBytes()
{
     QApplication::clipboard()->setText(ui->labelCoinControlBytes->text());
 }
 
//*****************************************************************************
// Coin Control: copy label "Priority" to clipboard
//*****************************************************************************
void SendCoinsDialog::coinControlClipboardPriority()
{
     QApplication::clipboard()->setText(ui->labelCoinControlPriority->text());
 }
 
//*****************************************************************************
// Coin Control: copy label "Low output" to clipboard
//*****************************************************************************
void SendCoinsDialog::coinControlClipboardLowOutput()
{
     QApplication::clipboard()->setText(ui->labelCoinControlLowOutput->text());
 }
 
//*****************************************************************************
// Coin Control: copy label "Change" to clipboard
//*****************************************************************************
void SendCoinsDialog::coinControlClipboardChange()
{
     QApplication::clipboard()->setText(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")));
 }
 
//*****************************************************************************
// Coin Control: settings menu - coin control enabled/disabled by user
//*****************************************************************************
void SendCoinsDialog::coinControlFeatureChanged(bool checked)
{
     ui->frameCoinControl->setVisible(checked);
 
     if (!checked && model) // coin control features disabled
         CoinControlDialog::coinControl->SetNull();
 }
 
//*****************************************************************************
// Coin Control: button inputs -> show actual coin control dialog
//*****************************************************************************
void SendCoinsDialog::coinControlButtonClicked()
{
     CoinControlDialog dlg;
     dlg.setModel(model);
     dlg.exec();
     coinControlUpdateLabels();
 }
 
//*****************************************************************************
// Coin Control: checkbox custom change address
//*****************************************************************************
void SendCoinsDialog::coinControlChangeChecked(int state)
{
     if (model)
     {
         if (state == Qt::Checked)
             CoinControlDialog::coinControl->destChange = CBitcoinAddress(ui->lineEditCoinControlChange->text().toStdString()).Get();
         else
             CoinControlDialog::coinControl->destChange = CNoDestination();
     }
 
     ui->lineEditCoinControlChange->setEnabled((state == Qt::Checked));
     ui->labelCoinControlChangeLabel->setEnabled((state == Qt::Checked));
 }
 
//*****************************************************************************
// Coin Control: custom change address changed
//*****************************************************************************
void SendCoinsDialog::coinControlChangeEdited(const QString & text)
{
     if (model)
     {
         CoinControlDialog::coinControl->destChange = CBitcoinAddress(text.toStdString()).Get();
 
         // label for the change address
         ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");
         if (text.isEmpty())
             ui->labelCoinControlChangeLabel->setText("");
         else if (!CBitcoinAddress(text.toStdString()).IsValid())
         {
             ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");
             ui->labelCoinControlChangeLabel->setText(tr("WARNING: Invalid Bitcoin address"));
         }
         else
         {
             QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
             if (!associatedLabel.isEmpty())
                 ui->labelCoinControlChangeLabel->setText(associatedLabel);
             else
             {
                 CPubKey pubkey;
                 CKeyID keyid;
                 CBitcoinAddress(text.toStdString()).GetKeyID(keyid);   
                 if (model->getPubKey(keyid, pubkey))
                     ui->labelCoinControlChangeLabel->setText(tr("(no label)"));
                 else
                 {
                     ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");
                     ui->labelCoinControlChangeLabel->setText(tr("WARNING: unknown change address"));
                 }
             }
         }
     }
 }
 
//*****************************************************************************
// Coin Control: update labels
//*****************************************************************************
void SendCoinsDialog::coinControlUpdateLabels()
{
     if (!model || !model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures())
         return;
     // set pay amounts
    CoinControlDialog::payAmounts.clear();
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
            CoinControlDialog::payAmounts.append(entry->getValue().amount);
    }
     if (CoinControlDialog::coinControl->HasSelected())
    {
        // actual coin control calculation
         CoinControlDialog::updateLabels(model, this);
        // show coin control stats
        ui->labelCoinControlAutomaticallySelected->hide();
        ui->widgetCoinControl->show();
    }
    else
    {
        // hide coin control stats
        ui->labelCoinControlAutomaticallySelected->show();
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
    }
}	

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::updateEntryStatus(const QString & status)
{
    // status:
    // mssearch
    // msjoin
    // mssuccess
    // msfailure
    // mstimeout

    if (!m_sendEntries.size())
    {
        return;
    }

    SendCoinsEntry * entry = m_sendEntries.front();

    // TODO
    // disable cancel

    if (status.compare("mssearch", Qt::CaseInsensitive) == 0)
    {
        entry->setStatus(trUtf8("searching"));

        // TODO
        // enable cancel
    }
    else if (status.compare("msprocess", Qt::CaseInsensitive) == 0)
    {
        entry->setStatus(trUtf8("processing"));

        // TODO
        // enable cancel
    }
    else if (status.compare("msjoin", Qt::CaseInsensitive) == 0)
    {
        entry->setStatus(trUtf8("signing"));
    }
    else if (status.compare("msreject", Qt::CaseInsensitive) == 0)
    {
        entry->setStatus(trUtf8("rejected"));

        QMessageBox::warning(this, trUtf8("Send Coins"),
            trUtf8("Rejected. Please try later/"),
            QMessageBox::Ok, QMessageBox::Ok);

        m_sendEntries.clear();

        setEnabledForProcessing(true);
    }
    else if (status.compare("mssuccess", Qt::CaseInsensitive) == 0)
    {
        entry->setStatus(trUtf8("done"));
        setEnabledForProcessing(true);      
        m_sendEntries.pop_front();

        // goto next payment
        if (m_sendEntries.size() != 0)
        {
            processDistmixPayments();
        }
		
		setEnabledForProcessing(true);
    }
    else if (status.compare("msfailure", Qt::CaseInsensitive) == 0)
    {
        entry->setStatus(trUtf8("failed"));

        m_sendEntries.pop_front();

        if (m_sendEntries.size() != 0)
        {
            if (QMessageBox::warning(this, trUtf8("Send Coins"),
                trUtf8("Failed, process next payments?"),
                QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
            {
                processDistmixPayments();
            }
            else
            {
                m_sendEntries.clear();
            }
        }
    }
    else if (status.compare("mstimeout", Qt::CaseInsensitive) == 0)
    {
        entry->setStatus(trUtf8("timeout expired"));

        m_sendEntries.pop_front();

        if (m_sendEntries.size() != 0)
        {
            if (QMessageBox::warning(this, trUtf8("Send Coins"),
                trUtf8("Timeout expired, process next payments?"),
                QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
            {
                processDistmixPayments();
            }
            else
            {
                m_sendEntries.clear();
            }
        }
    }
    else
    {
        // TODO
        // throw an exception?
    }

    // check all payments processed
    if (m_sendEntries.size() == 0)
    {
        setEnabledForProcessing(true);
    }
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::processDistmixPayments()
{
    const static std::string distmixCommand("distmix");

    int errCode = 0;
    std::string errMessage;

    try
    {
        SendCoinsRecipient rcp = m_sendEntries.front()->getValue();

        std::vector<std::string> params;
        params.push_back(rcp.address.toStdString());
        params.push_back(boost::lexical_cast<std::string>(rcp.amount/COIN));

        // call distmix
        tableRPC.execute(distmixCommand, RPCConvertValues(distmixCommand, params));
    }
    catch (json_spirit::Object & obj)
    {
        //
        errCode = find_value(obj, "code").get_int();
        errMessage = find_value(obj, "message").get_str();
		
    }
    catch (std::runtime_error & e)
    {
        // specified error
        errCode = -1;
        errMessage = e.what();
		setEnabledForProcessing(true);
    }
    catch (...)
    {
        errCode = -1;
        errMessage = "unknown error";
		setEnabledForProcessing(true);
    }

    if (errCode != 0)
    {
        QMessageBox::warning(this, trUtf8("Send Coins"),
            trUtf8("Failed, code %1\n%2").arg(QString::number(errCode), QString::fromStdString(errMessage)),
            QMessageBox::Ok, QMessageBox::Ok);

        setEnabledForProcessing(true);
    }
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::setEnabledForProcessing(const bool isEnabled)
{
    m_isNewRecipientAllowed = isEnabled;

    ui->addButton->setEnabled(isEnabled);
    ui->clearButton->setEnabled(isEnabled);
    ui->sendButton->setEnabled(isEnabled);
    ui->checkDistmix->setEnabled(isEnabled && !GetBoolArg("-distmix-autonode"));

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry * entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if (entry)
        {
            entry->setEnabledForProcessing(isEnabled);
        }
    }
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::on_checkDistmix_clicked()
{
    bool checked = (ui->txWithImage->checkState() == Qt::Checked);
    if (checked)
    {
        ui->txWithImage->setChecked(false);
    }
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::on_txWithImage_clicked()
{
    bool checked = (ui->txWithImage->checkState() == Qt::Checked);
    if (checked)
    {
        ui->checkDistmix->setChecked(false);
    }
    ui->imagePath->setEnabled(checked);
    ui->selectImageButton->setEnabled(checked);
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::on_selectImageButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    trUtf8("Select file"),
                                                    trUtf8("Select file for send"),
                                                    trUtf8("Document files (*.pdf)"));
                                                    // trUtf8("Image files (*.jpg *.jpeg);;All files (*.*)"));
    if (fileName.isEmpty())
        return;

    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);

    qDebug() << "MimeType:" << mimeType.name() << "parents:" << mimeType.parentMimeTypes() << "aliases" << mimeType.aliases();

    if(!mimeType.inherits(QLatin1String("application/pdf")))
    {
        QMessageBox::warning(this, tr("File type"),
            tr("File doesn't match PDF type."),
            QMessageBox::Ok);

        return;
    }

    ui->imagePath->setText(fileName);
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::processPaymentsWithImage()
{
    const static std::string createCommand("createrawtransaction");
    const static std::string signCommand("signrawtransaction");
    const static std::string sendCommand("sendrawtransaction");

    int errCode = 0;
    std::string errMessage;

    try
    {
//        std::map<QString, std::vector<COutput> > coins;
//        model->listCoins(coins);
//        for (const std::pair<QString, std::vector<COutput> > & item : coins)
//        {
//            for (const COutput & out : item.second)
//            {
//                amount += out.tx->vout[out.i].nValue;

//                used.push_back(out);

//                if (amount >= rcp.amount + nTransactionFee)
//                {
//                    break;
//                }
//            }

//            if (amount >= rcp.amount + nTransactionFee)
//            {
//                break;
//            }
//        }
        QFileInfo inf(ui->imagePath->text());

        Object outputs;
        {
            // add image
            if (!inf.exists())
            {
                throw std::runtime_error("File not found");
            }
            if (inf.size() > 1024*1024)
            {
                // size must be less than 1M
                throw std::runtime_error("File size too big");
            }

            {
                QFile f(inf.absoluteFilePath());
                if (!f.open(QFile::ReadOnly))
                {
                    throw std::runtime_error("File not opened");
                }

                QByteArray data = f.readAll();

                std::string strdata = HexStr(data.begin(), data.end());

                outputs.push_back(Pair("data", strdata));
            }
        }

        SendCoinsRecipient rcp = m_sendEntries.front()->getValue();
        outputs.push_back(Pair(rcp.address.toStdString(), (double)rcp.amount/COIN));

        // int64 nPayFee = (nTransactionFee == 0 ? MIN_TX_FEE : nTransactionFee) * (1 + (uint64_t)inf.size() / 1000);
        int64 nPayFee = (nTransactionFee == 0 ? MIN_TX_FEE : nTransactionFee) * (2 + (uint64_t)inf.size() / 1000);

        std::vector<COutput> vCoins;
        pwalletMain->AvailableCoins(vCoins, true, nullptr);
        // model->wallet->AvailableCoins(vCoins, true, nullptr);

        int64_t amount = 0;
        std::vector<COutput> used;

        for (const COutput & out : vCoins)
        {
            amount += out.tx->vout[out.i].nValue;

            used.push_back(out);

            if (amount >= rcp.amount + nPayFee)
            {
                break;
            }
        }

        if (amount < rcp.amount + nPayFee)
        {
            throw std::runtime_error("No money");
        }
        else if (amount > rcp.amount + nPayFee)
        {
            // rest
            CReserveKey rkey(pwalletMain);
            CPubKey pk = rkey.GetReservedKey();
            CBitcoinAddress addr(pk.GetID());
            uint64_t rest = amount - rcp.amount - nPayFee;
            outputs.push_back(Pair(addr.ToString(), (double)rest/COIN));
        }

        Array inputs;
        for (const COutput & out : used)
        {
            Object tmp;
            tmp.push_back(Pair("txid", out.tx->GetHash().ToString()));
            tmp.push_back(Pair("vout", out.i));
            inputs.push_back(tmp);
        }

        Value result;
        std::string rawtx;

        {
            Array params;
            params.push_back(inputs);
            params.push_back(outputs);

            // call create
            result = tableRPC.execute(createCommand, params);
            if (result.type() != str_type)
            {
                throw std::runtime_error("Create transaction command finished with error");
            }

            rawtx = result.get_str();
        }

        {
            std::vector<std::string> params;
            params.push_back(rawtx);

            result = tableRPC.execute(signCommand, RPCConvertValues(signCommand, params));
            if (result.type() != obj_type)
            {
                throw std::runtime_error("Sign transaction command finished with error");
            }

            Object obj = result.get_obj();
            const Value  & tx = find_value(obj, "hex");
            const Value & cpl = find_value(obj, "complete");

            if (tx.type() != str_type || cpl.type() != bool_type || !cpl.get_bool())
            {
                throw std::runtime_error("Sign transaction error or not completed");
            }

            rawtx = tx.get_str();
        }

        {
            std::vector<std::string> params;
            params.push_back(rawtx);

            result = tableRPC.execute(sendCommand, RPCConvertValues(sendCommand, params));
            if (result.type() != str_type)
            {
                throw std::runtime_error("Send transaction command finished with error");
            }
        }

        QMessageBox::information(this,
                                 trUtf8("Send Coins"),
                                 trUtf8("Done"),
                                 QMessageBox::Ok);
    }
    catch (json_spirit::Object & obj)
    {
        //
        errCode = find_value(obj, "code").get_int();
        errMessage = find_value(obj, "message").get_str();

    }
    catch (std::runtime_error & e)
    {
        // specified error
        errCode = -1;
        errMessage = e.what();
    }
    catch (...)
    {
        errCode = -1;
        errMessage = "unknown error";
    }

    if (errCode != 0)
    {
        QMessageBox::warning(this,
                             trUtf8("Send Coins"),
                             trUtf8("Failed, code %1\n%2").arg(QString::number(errCode), QString::fromStdString(errMessage)),
                             QMessageBox::Ok,
                             QMessageBox::Ok);
    }

    setEnabledForProcessing(true);
}
