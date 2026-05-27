// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/hidden_users/ayu_hidden_users.h"

#include "base/debug_log.h"
#include "core/file_utilities.h"
#include "main/main_session.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QStringConverter>

namespace Ayu::HiddenUsers {
namespace {

QSet<long long> g_hiddenUserIds;
Main::Session *g_session = nullptr;

} // namespace

void Init(not_null<Main::Session*> session) {
	g_session = session.get();
	
	// Default path: working directory or app data directory
	QString filePath = QDir::currentPath() + "/hidden_users.txt";
	QFileInfo fileInfo(filePath);
	if (!fileInfo.exists()) {
		// Try app data directory as fallback
		const auto appDataPath = QStandardPaths::writableLocation(
			QStandardPaths::AppDataLocation);
		filePath = appDataPath + "/hidden_users.txt";
	}
	
	LoadFromFile(filePath);
}

void LoadFromFile(const QString &filePath) {
	g_hiddenUserIds.clear();
	
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		LOG(("HiddenUsers: File not found at %1, creating empty list").arg(filePath));
		return;
	}
	
	QTextStream in(&file);
	in.setEncoding(QStringConverter::Utf8);
	int lineNumber = 0;
	while (!in.atEnd()) {
		lineNumber++;
		QString line = in.readLine().trimmed();
		
		// Skip empty lines and comments
		if (line.isEmpty() || line.startsWith('#') || line.startsWith("//")) {
			continue;
		}
		
		// Parse user ID (can be numeric string)
		bool ok = false;
		long long userId = line.toLongLong(&ok);
		if (ok && userId > 0) {
			g_hiddenUserIds.insert(userId);
		} else if (!line.isEmpty()) {
			LOG(("HiddenUsers: Invalid user ID at line %1: '%2'").arg(lineNumber).arg(line));
		}
	}
	
	file.close();
	
	LOG(("HiddenUsers: Loaded %1 hidden user IDs from %2")
		.arg(g_hiddenUserIds.size())
		.arg(filePath));
}

bool IsHidden(long long userId) {
	return g_hiddenUserIds.contains(userId);
}

QSet<long long> GetAllHidden() {
	return g_hiddenUserIds;
}

void Destroy() {
	g_hiddenUserIds.clear();
	g_session = nullptr;
}

} // namespace Ayu::HiddenUsers
