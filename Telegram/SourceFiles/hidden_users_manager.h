#pragma once

#include "data/data_peer_id.h"
#include <QFileSystemWatcher>
#include <QSet>
#include <QString>

class HiddenUsersManager {
public:
    static HiddenUsersManager &Instance();

    void loadFromFile();
    bool isHidden(PeerId peerId) const;

private:
    HiddenUsersManager();

    void watchFile(const QString &filePath);

    QSet<long long> _hiddenUserIds;
    QFileSystemWatcher _watcher;
};
