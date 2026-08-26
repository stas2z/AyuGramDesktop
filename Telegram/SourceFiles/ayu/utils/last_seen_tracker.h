#pragma once

#include "base/basic_types.h"
#include "data/data_peer_id.h"
#include <QDateTime>
#include <QHash>
#include <QString>
#include <optional>

class UserData;

// Approximates a user's last-seen time when Telegram doesn't provide
// one (privacy hidden), based on locally-observed activity: incoming
// messages, send-action pushes (typing/recording/uploading), and the
// arrival time of status-update pushes themselves. Best effort / "very
// approximately", never exact - see AyuSettings::saveLastSeenDate().
class LastSeenTracker {
public:
	static LastSeenTracker &Instance();

	void noteActivity(not_null<UserData*> user, TimeId when);
	[[nodiscard]] std::optional<TimeId> lastSeen(UserId userId) const;

private:
	LastSeenTracker();

	void load();
	void save() const;
	[[nodiscard]] static QString FilePath();

	QHash<uint64, TimeId> _lastSeen;
};
