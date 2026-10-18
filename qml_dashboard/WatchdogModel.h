#pragma once

#include <QAbstractListModel>
#include <QString>
#include <vector>

#include "watchdog.h"

/**
 * @class WatchdogModel
 * @brief Exposes a Watchdog's monitored entities to QML as a list model.
 *
 * Wraps Watchdog::getStatusSnapshot() behind the QAbstractListModel
 * interface so a QML ListView (or any other view) can bind to it directly.
 * The model does no locking of its own - getStatusSnapshot() already
 * returns a thread-safe copy, which refresh() stores and exposes via roles.
 */
class WatchdogModel : public QAbstractListModel {
    Q_OBJECT

public:
    /**
     * @brief Role IDs exposed to QML, mapped to role names by roleNames().
     */
    enum Roles {
        NameRole = Qt::UserRole + 1,  ///< Display name, e.g. "proc (pid=1234)".
        StateRole,                    ///< Status label, e.g. "ALIVE", "STUCK".
        MemoryRole,                   ///< Resident memory usage in kilobytes.
        CpuRole                       ///< CPU usage percentage.
    };

    /**
     * @brief Constructs a WatchdogModel backed by the given Watchdog.
     * @param watchdog Reference to the Watchdog to read snapshots from. Must
     *                 outlive this model.
     * @param parent Optional QObject parent.
     */
    explicit WatchdogModel(Watchdog& watchdog, QObject* parent = nullptr);

    /**
     * @brief Returns the number of monitored entities in the current snapshot.
     * @param parent Unused; list models have no parent-child rows.
     * @return Row count.
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Returns the value for a given row and role.
     * @param index Row to query.
     * @param role One of the Roles enum values.
     * @return The requested value, or an invalid QVariant if out of range.
     */
    QVariant data(const QModelIndex& index, int role) const override;

    /**
     * @brief Maps each Roles value to the property name QML binds against.
     * @return Role ID to name mapping (e.g. NameRole -> "name").
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Sends SIGTERM to the monitored entity with the given name.
     * @param name Display name as shown by the name role.
     * @return true if a matching entity was found and killed.
     */
    Q_INVOKABLE bool kill(const QString& name);

    /**
     * @brief Kills and relaunches the monitored entity with the given name.
     * @param name Display name as shown by the name role.
     * @return true if a matching entity was found and restarted.
     */
    Q_INVOKABLE bool restart(const QString& name);

public slots:
    /**
     * @brief Re-pulls a fresh snapshot from the Watchdog and updates the model.
     *
     * Only emits dataChanged() for roles that actually changed, so an
     * unchanged list doesn't flicker on every tick. Falls back to a full
     * beginResetModel()/endResetModel() if a row was added, removed, or
     * reordered.
     */
    void refresh();

private:
    Watchdog& m_watchdog;
    std::vector<WatchableStatus> m_snapshot;
};
