#include "ayu/utils/last_seen_tracker.h"

#include "data/data_user.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

namespace {

QString FallbackDir() {
	auto dir = QStandardPaths::writableLocation(
		QStandardPaths::AppDataLocation);
	if (dir.isEmpty()) {
		dir = QDir::currentPath();
	}
	return dir;
}

} // namespace

LastSeenTracker &LastSeenTracker::Instance() {
	static LastSeenTracker instance;
	return instance;
}

LastSeenTracker::LastSeenTracker() {
	load();
}

QString LastSeenTracker::FilePath() {
	return FallbackDir() + "/ayu_last_seen_cache.txt";
}

void LastSeenTracker::noteActivity(
		not_null<UserData*> user,
		TimeId when) {
	if (when <= 0) {
		return;
	}
	const auto id = user->id.value;
	const auto it = _lastSeen.constFind(id);
	if (it != _lastSeen.constEnd() && it.value() >= when) {
		return;
	}
	_lastSeen[id] = when;
	save();
}

std::optional<TimeId> LastSeenTracker::lastSeen(UserId userId) const {
	const auto id = PeerId(userId).value;
	const auto it = _lastSeen.constFind(id);
	return (it != _lastSeen.constEnd())
		? std::make_optional(it.value())
		: std::nullopt;
}

void LastSeenTracker::load() {
	QFile file(FilePath());
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return;
	}
	QTextStream in(&file);
	in.setEncoding(QStringConverter::Utf8);
	while (!in.atEnd()) {
		const auto line = in.readLine().trimmed();
		if (line.isEmpty()) {
			continue;
		}
		const auto parts = line.split(' ');
		if (parts.size() != 2) {
			continue;
		}
		bool idOk = false, timeOk = false;
		const auto id = parts[0].toULongLong(&idOk);
		const auto when = parts[1].toLongLong(&timeOk);
		if (idOk && timeOk && when > 0) {
			_lastSeen[id] = TimeId(when);
		}
	}
}

void LastSeenTracker::save() const {
	QFile file(FilePath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return;
	}
	QTextStream out(&file);
	out.setEncoding(QStringConverter::Utf8);
	for (auto it = _lastSeen.constBegin(); it != _lastSeen.constEnd(); ++it) {
		out << it.key() << ' ' << qint64(it.value()) << '\n';
	}
}
