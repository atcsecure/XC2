//******************************************************************************
//******************************************************************************

#include "xbridgeaddressbookview.h"

#include <QHBoxLayout>
#include <QHeaderView>

//******************************************************************************
//******************************************************************************
XBridgeAddressBookView::XBridgeAddressBookView(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
}

//******************************************************************************
//******************************************************************************
XBridgeAddressBookView::~XBridgeAddressBookView()
{

}

//******************************************************************************
//******************************************************************************
void XBridgeAddressBookView::setupUi()
{
    m_entryList = new QTableView(this);
    m_entryList->setModel(&m_model);
    m_entryList->setSelectionBehavior(QAbstractItemView::SelectRows);

//    QHeaderView * header = m_entryList->horizontalHeader();
//#if QT_VERSION <0x050000
//    header->setResizeMode(XBridgeAddressBookModel::Currency, QHeaderView::Stretch);
//#else
//    header->setSectionResizeMode(XBridgeAddressBookModel::Currency, QHeaderView::Stretch);
//#endif
//    header->resizeSection(XBridgeAddressBookModel::Name,  80);
//#if QT_VERSION <0x050000
//    header->setResizeMode(XBridgeAddressBookModel::Address, QHeaderView::Stretch);
//#else
//    header->setSectionResizeMode(XBridgeAddressBookModel::Address, QHeaderView::Stretch);
//#endif

    QHBoxLayout * hbox = new QHBoxLayout;
    hbox->addWidget(m_entryList);

    setLayout(hbox);
}
