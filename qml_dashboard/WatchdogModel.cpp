#include "WatchdogModel.h"

WatchdogModel::WatchdogModel(Watchdog& watchdog, QObject* parent)
    : QAbstractListModel(parent), m_watchdog(watchdog) {
    refresh();
}

int WatchdogModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_snapshot.size());
}

QVariant WatchdogModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_snapshot.size())) {
        return {};
    }

    const WatchableStatus& status = m_snapshot[index.row()];

    switch (role) {
        case NameRole:
            return QString::fromStdString(status.name);
        case StateRole:
            return QString::fromStdString(status.statusLabel);
        case MemoryRole:
            return static_cast<qlonglong>(status.memoryKB);
        case CpuRole:
            return status.cpuPercent;
        default:
            return {};
    }
}

QHash<int, QByteArray> WatchdogModel::roleNames() const {
    return {
        {NameRole, "name"},
        {StateRole, "state"},
        {MemoryRole, "memory"},
        {CpuRole, "cpu"},
    };
}

void WatchdogModel::refresh() {
    beginResetModel();
    m_snapshot = m_watchdog.getStatusSnapshot();
    endResetModel();
}
