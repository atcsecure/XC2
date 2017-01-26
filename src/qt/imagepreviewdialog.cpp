//*****************************************************************************
//*****************************************************************************

#include "imagepreviewdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

//*****************************************************************************
//*****************************************************************************
ImagePreviewDialog::ImagePreviewDialog(const QPixmap & pix, QWidget * parent)
    : QDialog(parent)
    , m_pix(pix)
{
    createUI();
}

//*****************************************************************************
//*****************************************************************************
void ImagePreviewDialog::createUI()
{
    setMinimumSize(100, 100);

    QVBoxLayout * vbox = new QVBoxLayout;

    {
        QLabel * l = new QLabel(this);
        l->setPixmap(m_pix);
        vbox->addWidget(l);
    }

    QHBoxLayout * hbox = new QHBoxLayout;
    hbox->addStretch();

    QPushButton * close = new QPushButton(trUtf8("Close"), this);
    hbox->addWidget(close);

    vbox->addLayout(hbox);

    setLayout(vbox);
}
