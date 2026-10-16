#pragma once

#include <QAbstractListModel>
#include <vector>

#include "watchdog.h"

class WatchdogModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        StateRole,
        MemoryRole,
        CpuRole
    };

    explicit WatchdogModel(Watchdog& watchdog, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();

private:
    Watchdog& m_watchdog;
    std::vector<WatchableStatus> m_snapshot;
};
