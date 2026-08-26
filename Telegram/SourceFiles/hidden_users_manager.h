#pragma once

#include "base/basic_types.h"
#include "data/data_peer_id.h"
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QSet>
#include <QString>
#include <QTimer>

class PeerData;

class HiddenUsersManager {
public:
    static HiddenUsersManager &Instance();

    void loadFromFile();
    bool isHidden(PeerId peerId) const;
    bool hasAny() const;

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

namespace HiddenUsers {

// Best-effort member count with hidden users excluded, using only
// locally-known participants (chat->participants for basic groups,
// mgInfo->lastParticipants for megagroups - full member lists aren't
// always loaded, so this can't be exact for large/unloaded groups).
// rawCount is the peer's own already-known counter (chat->count /
// channel->membersCount()) to subtract from.
[[nodiscard]] int VisibleMembersCount(
	not_null<PeerData*> peer,
	int rawCount);

// Whether we can trust VisibleMembersCount() not to still include a
// hidden user we simply haven't loaded yet - true when no hidden
// users are configured at all (nothing to leak), or when the peer's
// participant list is fully loaded. Callers should avoid showing a
// raw member count until this is true, to prevent a brief flash of
// the un-subtracted number before participants finish loading.
[[nodiscard]] bool CountKnown(not_null<PeerData*> peer);

} // namespace HiddenUsers
