#include "jsonconstructeddialog.h"
#include "util/verify.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

JsonConstructedDialog::JsonConstructedDialog(const QJsonDocument &json, QWidget *parent)
    : QDialog(parent)
    , m_json(json)
{
    createUI();
}

JsonConstructedDialog::JsonConstructedDialog(const QByteArray &ba, QWidget *parent)
    : QDialog(parent)
{
    m_json = QJsonDocument::fromBinaryData(ba);
    createUI();
}

void JsonConstructedDialog::createUI()
{
    setMinimumSize(300, 300);

    QVBoxLayout* vbox = new QVBoxLayout;

    constructFromJson(m_json.object(), vbox);

    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->addStretch();

    QPushButton* close = new QPushButton(trUtf8("Close"), this);
    VERIFY(connect(close, &QPushButton::clicked,
                   this,  &JsonConstructedDialog::reject));
    hbox->addWidget(close);

    vbox->addLayout(hbox);

    setLayout(vbox);
}

void JsonConstructedDialog::constructFromJson(const QJsonObject &json, QBoxLayout *layout)
{
    for(auto key : json.keys())
    {
        if(key == QStringLiteral("HBoxLayout"))
        {
            QHBoxLayout* hbox = new QHBoxLayout;
            hbox->addStretch();
            layout->addLayout(hbox);
            constructFromJson(json.value(key).toObject(), hbox);
        }
        if(key == QStringLiteral("Label"))
        {
            QLabel* label = new QLabel(this);
            label->setText(json.value(key).toObject().value(QStringLiteral("text")).toString());
            layout->addWidget(label);
        }
        if(key == QStringLiteral("LineEdit"))
        {
            QLineEdit* lineEdit = new QLineEdit(this);
            lineEdit->setText(json.value(key).toObject().value(QStringLiteral("text")).toString());
            layout->addWidget(lineEdit);
        }
    }
}
