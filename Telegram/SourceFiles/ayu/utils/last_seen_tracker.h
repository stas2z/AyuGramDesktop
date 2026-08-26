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
// messages, send-action pushes (typing/recording/uploading), read
// receipts, and posting a story. Best effort / "very approximately",
// never exact - see AyuSettings::saveLastSeenDate().
class LastSeenTracker {
public:
	struct Point {
		TimeId when = 0;
		// Short, fixed label identifying what was detected (e.g.
		// "message", "typing", "read", "story") - shown to the user
		// alongside the approximate time so it's clear it's a guess
		// and what it's based on.
		QString reason;
	};

	static LastSeenTracker &Instance();

	void noteActivity(
		not_null<UserData*> user,
		TimeId when,
		const QString &reason);
	[[nodiscard]] std::optional<Point> lastSeen(UserId userId) const;

private:
	LastSeenTracker();

	void load();
	void save() const;
	[[nodiscard]] static QString FilePath();

	QHash<uint64, Point> _lastSeen;
};
