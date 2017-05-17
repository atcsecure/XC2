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
    void constructFromJson(const QJsonArray &jsonArray, QBoxLayout *layout);

    QString getString(const QString& key, const QJsonObject& json) const;
    bool getBool(const QString& key, const QJsonObject& json) const;

private:
    QJsonDocument m_json;
};

#endif // JSONCONSTRUCTEDDIALOG_H
