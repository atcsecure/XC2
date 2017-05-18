#ifndef JSONCONSTRUCTEDDIALOG_H
#define JSONCONSTRUCTEDDIALOG_H

#include <QDialog>
#include <QJsonDocument>
#include <QByteArray>
#include <QVBoxLayout>
#include <QHBoxLayout>

class JsonConstructedDialog : public QDialog
{
    Q_OBJECT
public:
    JsonConstructedDialog(const QJsonDocument& json, bool editable = false, QWidget* parent = 0);
    JsonConstructedDialog(const QByteArray& ba, bool editable = false, QWidget* parent = 0);

signals:
    void formEdited(QJsonDocument json);

private:
    void createUI();
    void constructFromJson(const QJsonArray &jsonArray, QBoxLayout *layout);
    void objectToJson(QJsonArray &jsonArray, QObject* parentObject) const;

    QString getString(const QString& key, const QJsonObject& json) const;
    bool getBool(const QString& key, const QJsonObject& json) const;

private:
    QJsonDocument m_json;
    bool m_editable = false;
};

#endif // JSONCONSTRUCTEDDIALOG_H
