//*****************************************************************************
//*****************************************************************************

#ifndef IMAGEPREVIEWDIALOG_H
#define IMAGEPREVIEWDIALOG_H

#include <QDialog>
#include <QPixmap>

//*****************************************************************************
//*****************************************************************************
class ImagePreviewDialog : public QDialog
{
public:
    ImagePreviewDialog(const QPixmap & pix, QWidget * parent = 0);

private:
    void createUI();

private:
    QPixmap m_pix;
};

#endif // IMAGEPREVIEWDIALOG_H
