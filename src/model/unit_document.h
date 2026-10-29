#pragma once

#include <QObject>
#include <QUuid>

// Live-scene notifications. Unit stays copyable and is not a QObject.
// Payloads are UUIDs because array rebuilds invalidate Unit pointers.
class UnitDocument : public QObject
{
    Q_OBJECT

public:
    explicit UnitDocument(QObject *parent = nullptr);

signals:
    void poseChanged(const QUuid &uuid);
    void directionChanged(const QUuid &uuid);
    void followChanged(const QUuid &uuid);
    void presentationChanged(const QUuid &uuid);
    void structureChanged();
    void contentChanged(const QUuid &uuid);
    void unitAdded(const QUuid &uuid);
    void unitRemoved(const QUuid &uuid);
};
