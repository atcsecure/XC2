//******************************************************************************
//******************************************************************************

#ifndef XBRIDGEADDRESSBOOKVIEW_H
#define XBRIDGEADDRESSBOOKVIEW_H

#include "xbridgeaddressbookmodel.h"

#include <QDialog>
#include <QTableView>

//******************************************************************************
//******************************************************************************
class XBridgeAddressBookView : public QDialog
{
    Q_OBJECT
public:
    explicit XBridgeAddressBookView(QWidget *parent = 0);
    ~XBridgeAddressBookView();

    std::string selectedAddress() const { return m_selectedAddress; }


private slots:
    void onAddressSelect(QModelIndex index);

private:
    void setupUi();

private:
    std::string m_selectedAddress;

    XBridgeAddressBookModel m_model;

    QTableView  * m_entryList;
};

#endif // XBRIDGEADDRESSBOOKVIEW_H
