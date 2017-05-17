#ifndef JSONCONSTRUCTEDDIALOG_H
#define JSONCONSTRUCTEDDIALOG_H

#include <QDialog>
#include <QJsonDocument>
#include <QByteArray>
#include <QVBoxLayout>
#include <QHBoxLayout>

class JsonConstructedDialog : public QDialog
{
public:
    JsonConstructedDialog(const QJsonDocument& json, QWidget* parent = 0);
    JsonConstructedDialog(const QByteArray& ba, QWidget* parent = 0);

private:
    void createUI();
    void constructFromJson(const QJsonObject& json, QBoxLayout *layout);

private:
    QJsonDocument m_json;
};

#endif // JSONCONSTRUCTEDDIALOG_H
