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

bool WatchdogModel::kill(const QString& name) {
    bool success = m_watchdog.killByName(name.toStdString());
    refresh();
    return success;
}

bool WatchdogModel::restart(const QString& name) {
    bool success = m_watchdog.restartByName(name.toStdString());
    refresh();
    return success;
}

void WatchdogModel::refresh() {
    std::vector<WatchableStatus> newSnapshot = m_watchdog.getStatusSnapshot();
    
    bool structureChanged = newSnapshot.size() != m_snapshot.size();
    for (std::size_t i = 0; !structureChanged && i < newSnapshot.size(); ++i) {
        if (newSnapshot[i].name != m_snapshot[i].name) {
            structureChanged = true;
        }
    }

    if (structureChanged) {
        beginResetModel();
        m_snapshot = std::move(newSnapshot);
        endResetModel();
        return;
    }

    for (int row = 0; row < static_cast<int>(newSnapshot.size()); ++row) {
        const WatchableStatus& oldStatus = m_snapshot[static_cast<std::size_t>(row)];
        const WatchableStatus& newStatus = newSnapshot[static_cast<std::size_t>(row)];

        QList<int> changedRoles;
        if (oldStatus.statusLabel != newStatus.statusLabel) changedRoles.append(StateRole);
        if (oldStatus.memoryKB != newStatus.memoryKB) changedRoles.append(MemoryRole);
        if (oldStatus.cpuPercent != newStatus.cpuPercent) changedRoles.append(CpuRole);

        if (!changedRoles.isEmpty()) {
            const QModelIndex changedIndex = index(row);
            emit dataChanged(changedIndex, changedIndex, changedRoles);
        }
    }

    m_snapshot = std::move(newSnapshot);
}
