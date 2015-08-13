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

signals:

public slots:

private:
    void setupUi();

private:
    XBridgeAddressBookModel m_model;

    QTableView  * m_entryList;
};

#endif // XBRIDGEADDRESSBOOKVIEW_H
