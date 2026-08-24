#pragma once

#include "data/data_peer_id.h"
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QSet>
#include <QString>
#include <QTimer>

class HiddenUsersManager {
public:
    static HiddenUsersManager &Instance();

    void loadFromFile();
    bool isHidden(PeerId peerId) const;

private:
    HiddenUsersManager();

    void watchFile(const QString &filePath);
    void checkForChanges();

    QSet<long long> _hiddenUserIds;
    QFileSystemWatcher _watcher;
    // QFileSystemWatcher (inotify) is unreliable across some setups
    // (sandboxed/Flatpak/AppImage environments, network filesystems,
    // editors that rewrite via rename), so a periodic mtime poll backs
    // it up to guarantee changes are eventually picked up.
    QTimer _pollTimer;
    QString _watchedPath;
    QDateTime _lastModified;
};
