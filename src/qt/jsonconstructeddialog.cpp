#include "jsonconstructeddialog.h"
#include "util/verify.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QJsonParseError>
#include <QMessageBox>

JsonConstructedDialog::JsonConstructedDialog(const QJsonDocument &json, QWidget *parent)
    : QDialog(parent)
    , m_json(json)
{
    createUI();
}

JsonConstructedDialog::JsonConstructedDialog(const QByteArray &ba, QWidget *parent)
    : QDialog(parent)
{
    QJsonParseError error;
    m_json = QJsonDocument::fromJson(ba, &error);

    if(error.error != QJsonParseError::NoError)
    {
        QMessageBox::warning(this, trUtf8("Parsing error"), error.errorString());
        return;
    }

    createUI();
}

void JsonConstructedDialog::createUI()
{
    setMinimumSize(300, 300);

    QVBoxLayout* vbox = new QVBoxLayout;

    QJsonValue elementsValue = m_json.object().value(QStringLiteral("elements"));
    if(elementsValue.isArray())
        constructFromJson(elementsValue.toArray(), vbox);
    else
    {
        QMessageBox::warning(this, trUtf8("Parsing error"), trUtf8("Can't find root element"));
        return;
    }

    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->addStretch();

    QPushButton* close = new QPushButton(trUtf8("Close"), this);
    VERIFY(connect(close, &QPushButton::clicked,
                   this,  &JsonConstructedDialog::reject));
    hbox->addWidget(close);

    vbox->addLayout(hbox);

    setLayout(vbox);
}

void JsonConstructedDialog::constructFromJson(const QJsonArray &jsonArray, QBoxLayout *layout)
{
    for(QJsonValue value : jsonArray)
    {
        if(!value.isObject())
            continue;

        QJsonObject object = value.toObject();

        QString type = object.value(QStringLiteral("type")).toString();

        if(type == QStringLiteral("Title"))
        {
            setWindowTitle(getString(QStringLiteral("text"), object));
        }
        else if(type == QStringLiteral("VBoxLayout"))
        {
            QJsonValue elementsValue = object.value(QStringLiteral("elements"));
            if(!elementsValue.isArray())
                continue;

            QVBoxLayout* vbox = new QVBoxLayout;
            vbox->addStretch();
            layout->addLayout(vbox);

            constructFromJson(elementsValue.toArray(), vbox);
        }
        else if(type == QStringLiteral("HBoxLayout"))
        {
            QJsonValue elementsValue = object.value(QStringLiteral("elements"));
            if(!elementsValue.isArray())
                continue;

            QHBoxLayout* hbox = new QHBoxLayout;
            hbox->addStretch();
            layout->addLayout(hbox);

            constructFromJson(elementsValue.toArray(), hbox);
        }
        else if(type == QStringLiteral("Label"))
        {
            QLabel* label = new QLabel(this);
            label->setText(getString(QStringLiteral("text"), object));
            QFont font = label->font();
            font.setBold(getBool(QStringLiteral("bold"), object));
            label->setFont(font);

            layout->addWidget(label);
        }
        else if(type == QStringLiteral("LineEdit"))
        {
            QLineEdit* lineEdit = new QLineEdit(this);
            lineEdit->setText(getString(QStringLiteral("text"), object));
            lineEdit->setReadOnly(getBool(QStringLiteral("readOnly"), object));

            layout->addWidget(lineEdit);
        }
        else if(type == QStringLiteral("CheckBox"))
        {
            QCheckBox* checkBox = new QCheckBox(this);
            checkBox->setText(getString(QStringLiteral("text"), object));
            checkBox->setChecked(getBool(QStringLiteral("checked"), object));
            if(!getBool(QStringLiteral("checkable"), object))
            {
                checkBox->setAttribute(Qt::WA_TransparentForMouseEvents);
                checkBox->setFocusPolicy(Qt::NoFocus);
            }

            layout->addWidget(checkBox);
        }
    }
}

QString JsonConstructedDialog::getString(const QString &key, const QJsonObject &json) const
{
    return json.value(key).toString();
}

bool JsonConstructedDialog::getBool(const QString &key, const QJsonObject &json) const
{
    return json.value(key).toBool();
}
