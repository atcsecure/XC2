#include "jsonconstructeddialog.h"
#include "util/verify.h"

#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QJsonParseError>
#include <QMessageBox>

JsonConstructedDialog::JsonConstructedDialog(const QJsonDocument &json, bool editable, QWidget *parent)
    : QDialog(parent)
    , m_json(json)
    , m_editable(editable)
{
    createUI();
}

JsonConstructedDialog::JsonConstructedDialog(const QByteArray &ba, bool editable, QWidget *parent)
    : QDialog(parent)
    , m_editable(editable)
{
    QJsonParseError error;
    QByteArray strippedJson = ba;
    strippedJson.replace(QStringLiteral("\r\n"), "");
    m_json = QJsonDocument::fromJson(strippedJson, &error);

    if(error.error != QJsonParseError::NoError)
    {
        QMessageBox::warning(this, trUtf8("Parsing error"), error.errorString());
        return;
    }

    createUI();
}

void JsonConstructedDialog::createUI()
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout* vbox = new QVBoxLayout;

    QJsonValue windowSizeValue = m_json.object().value(QStringLiteral("windowSize"));
    if(windowSizeValue.isObject())
    {
        QJsonObject windowSizeObject = windowSizeValue.toObject();
        int x = windowSizeObject.value(QStringLiteral("x")).toInt(300);
        int y = windowSizeObject.value(QStringLiteral("y")).toInt(300);

        setMinimumSize(x, y);
    }
    else
        setMinimumSize(300, 300);

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

    if(m_editable)
    {
        QPushButton* save = new QPushButton(trUtf8("Save"), this);
        VERIFY(connect(save, &QPushButton::clicked, [=](bool){
            QJsonArray elements;
            objectToJson(elements, vbox);

            QJsonObject windowSize;
            windowSize[QStringLiteral("x")] = this->width();
            windowSize[QStringLiteral("y")] = this->height();

            QJsonObject json;
            json[QStringLiteral("windowSize")] = windowSize;
            json[QStringLiteral("elements")] = elements;

            QJsonDocument document;
            document.setObject(json);
            emit formEdited(document);
            emit accept();
        }));
        hbox->addWidget(save);
    }
    else
    {
        QPushButton* close = new QPushButton(trUtf8("Close"), this);
        VERIFY(connect(close, &QPushButton::clicked, this,  &JsonConstructedDialog::reject));
        hbox->addWidget(close);
    }

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
            layout->addLayout(vbox);

            constructFromJson(elementsValue.toArray(), vbox);
        }
        else if(type == QStringLiteral("HBoxLayout"))
        {
            QJsonValue elementsValue = object.value(QStringLiteral("elements"));
            if(!elementsValue.isArray())
                continue;

            QHBoxLayout* hbox = new QHBoxLayout;
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
            lineEdit->setReadOnly(!m_editable);

            layout->addWidget(lineEdit);
        }
        else if(type == QStringLiteral("TextEdit"))
        {
            QPlainTextEdit* plainTextEdit = new QPlainTextEdit(this);
            plainTextEdit->setPlainText(getString(QStringLiteral("text"), object));
            plainTextEdit->setReadOnly(!m_editable);

            layout->addWidget(plainTextEdit);
        }
        else if(type == QStringLiteral("CheckBox"))
        {
            QCheckBox* checkBox = new QCheckBox(this);
            checkBox->setText(getString(QStringLiteral("text"), object));
            checkBox->setChecked(getBool(QStringLiteral("checked"), object));
            if(!m_editable)
            {
                checkBox->setAttribute(Qt::WA_TransparentForMouseEvents);
                checkBox->setFocusPolicy(Qt::NoFocus);
            }

            layout->addWidget(checkBox);
        }
    }
}

void JsonConstructedDialog::objectToJson(QJsonArray &jsonArray, QObject* parentObject) const
{
    QObjectList childrens;

    if(parentObject->inherits(QLayout::staticMetaObject.className()))
    {
        QLayout *layout = qobject_cast<QLayout*>(parentObject);
        for(int i = 0; i < layout->count(); ++i)
        {
            QObject* layoutObject = layout->itemAt(i)->layout();
            QObject* widgetObject = layout->itemAt(i)->widget();

            if(layoutObject)
                childrens.append(layoutObject);
            else if(widgetObject)
                childrens.append(widgetObject);
        }
    }
    else
        childrens = parentObject->children();

    for(auto children : childrens)
    {
        if(children->inherits(QVBoxLayout::staticMetaObject.className()))
        {
            QVBoxLayout *object = qobject_cast<QVBoxLayout*>(children);

            if(!object)
                continue;

            QJsonArray elements;
            objectToJson(elements, children);

            if(elements.isEmpty())
                continue;

            QJsonObject json;
            json[QStringLiteral("type")] = QStringLiteral("VBoxLayout");
            json[QStringLiteral("elements")] = elements;

            jsonArray.append(json);
        }
        else if(children->inherits(QHBoxLayout::staticMetaObject.className()))
        {
            QHBoxLayout *object = qobject_cast<QHBoxLayout*>(children);

            if(!object)
                continue;

            QJsonArray elements;
            objectToJson(elements, children);

            if(elements.isEmpty())
                continue;

            QJsonObject json;
            json[QStringLiteral("type")] = QStringLiteral("HBoxLayout");
            json[QStringLiteral("elements")] = elements;

            jsonArray.append(json);
        }
        else if(children->inherits(QLabel::staticMetaObject.className()))
        {
            QLabel *object = qobject_cast<QLabel*>(children);

            if(!object)
                continue;

            QJsonObject json;
            json[QStringLiteral("type")] = QStringLiteral("Label");
            json[QStringLiteral("text")] = object->text();
            json[QStringLiteral("bold")] = object->font().bold();

            jsonArray.append(json);
        }
        else if(children->inherits(QLineEdit::staticMetaObject.className()))
        {
            QLineEdit *object = qobject_cast<QLineEdit*>(children);

            if(!object)
                continue;

            QJsonObject json;
            json[QStringLiteral("type")] = QStringLiteral("LineEdit");
            json[QStringLiteral("text")] = object->text();

            jsonArray.append(json);
        }
        else if(children->inherits(QPlainTextEdit::staticMetaObject.className()))
        {
            QPlainTextEdit *object = qobject_cast<QPlainTextEdit*>(children);

            if(!object)
                continue;

            QJsonObject json;
            json[QStringLiteral("type")] = QStringLiteral("TextEdit");
            json[QStringLiteral("text")] = object->toPlainText();

            jsonArray.append(json);
        }
        else if(children->inherits(QCheckBox::staticMetaObject.className()))
        {
            QCheckBox *object = qobject_cast<QCheckBox*>(children);

            if(!object)
                continue;

            QJsonObject json;
            json[QStringLiteral("type")] = QStringLiteral("CheckBox");
            json[QStringLiteral("text")] = object->text();
            json[QStringLiteral("checked")] = object->isChecked();

            jsonArray.append(json);
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
