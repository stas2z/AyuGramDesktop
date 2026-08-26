#include "hidden_users_manager.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_peer_id.h"
#include "data/data_user.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QSet>
#include <QStandardPaths>
#include <algorithm>

namespace {

QString GetFilePath() {
    // Основной путь: текущая директория (для portable версии или разработки)
    QString filePath = QDir::currentPath() + "/hidden_users.txt";
    QFileInfo fileInfo(filePath);
    
    if (!fileInfo.exists()) {
        // Альтернативный путь: директория данных приложения
        const auto appDataPath = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);
        filePath = appDataPath + "/hidden_users.txt";
    }
    
    return filePath;
}

} // namespace

HiddenUsersManager &HiddenUsersManager::Instance() {
    static HiddenUsersManager instance;
    return instance;
}

HiddenUsersManager::HiddenUsersManager() {
    // Инициализация будет выполнена при создании сессии
}

void HiddenUsersManager::watchFile(const QString &filePath) {
    _watchedPath = filePath;
    _lastModified = QFileInfo(filePath).lastModified();

    // QFileSystemWatcher drops a path once the underlying file is removed
    // (e.g. an editor doing save-as-rename), so re-add it every time we
    // (re)load - addPath() is a no-op if it's already watched.
    if (!_watcher.files().contains(filePath)) {
        _watcher.addPath(filePath);
    }
    static auto connected = false;
    if (!connected) {
        connected = true;
        QObject::connect(
            &_watcher,
            &QFileSystemWatcher::fileChanged,
            [this](const QString &) {
                qDebug() << "[HiddenUsers] hidden_users.txt changed (watcher), reloading";
                loadFromFile();
            });
        QObject::connect(
            &_pollTimer,
            &QTimer::timeout,
            [this] { checkForChanges(); });
        // Backup poll in case QFileSystemWatcher (inotify) misses the
        // change - e.g. sandboxed/Flatpak/AppImage runs, network
        // filesystems, or editors that replace the file via rename in
        // a way the watcher doesn't reliably pick back up.
        _pollTimer.start(3000);
    }
}

void HiddenUsersManager::checkForChanges() {
    if (_watchedPath.isEmpty()) {
        return;
    }
    const auto info = QFileInfo(_watchedPath);
    if (!info.exists()) {
        return;
    }
    const auto modified = info.lastModified();
    if (modified.isValid() && modified != _lastModified) {
        qDebug() << "[HiddenUsers] hidden_users.txt changed (poll), reloading";
        loadFromFile();
    }
}

void HiddenUsersManager::loadFromFile() {
    const auto filePath = GetFilePath();

    qDebug() << "[HiddenUsers] Attempting to load from:" << filePath;

    _hiddenUserIds.clear();

    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "[HiddenUsers] File does not exist. Will use empty list.";
        qDebug() << "[HiddenUsers] You can create" << filePath << "with one user ID per line";
        qDebug() << "[HiddenUsers] Lines starting with # or // are treated as comments";
        return;
    }

    watchFile(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[HiddenUsers] Failed to open file:" << filePath;
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    int lineNumber = 0;
    int validCount = 0;
    int invalidCount = 0;

    while (!in.atEnd()) {
        lineNumber++;
        QString line = in.readLine().trimmed();
        
        // Пропускаем пустые строки и комментарии
        if (line.isEmpty() || line.startsWith('#') || line.startsWith("//")) {
            continue;
        }
        
        // Парсим ID пользователя (числовой формат)
        bool ok = false;
        long long userId = line.toLongLong(&ok);
        if (ok && userId > 0) {
            _hiddenUserIds.insert(userId);
            validCount++;
        } else if (!line.isEmpty()) {
            qDebug() << "[HiddenUsers] Invalid user ID at line" << lineNumber << ":" << line;
            invalidCount++;
        }
    }
    
    file.close();
    
    qDebug() << "[HiddenUsers] Successfully loaded" << validCount << "hidden user IDs";
    if (invalidCount > 0) {
        qDebug() << "[HiddenUsers] Skipped" << invalidCount << "invalid lines";
    }
    
    // Выводим первые несколько ID для отладки
    auto list = _hiddenUserIds.values();
    const int previewCount = std::min<int>(10, list.size());
    if (previewCount > 0) {
        qDebug() << "[HiddenUsers] First" << previewCount << "IDs:";
        for(int i = 0; i < previewCount; ++i) {
            qDebug() << "  -" << list[i];
        }
    }
}

bool HiddenUsersManager::isHidden(PeerId peerId) const {
    if (!peerIsUser(peerId)) {
        return false;
    }
    
    // Получаем bare ID пользователя
    const auto userId = peerToUser(peerId).bare;
    
    bool result = _hiddenUserIds.contains(userId);
    if (result) {
        qDebug() << "[HiddenUsers] User is hidden:" << userId;
    }
    return result;
}

namespace HiddenUsers {

int VisibleMembersCount(not_null<PeerData*> peer, int rawCount) {
	auto hidden = 0;
	if (const auto chat = peer->asChat()) {
		for (const auto &user : chat->participants) {
			if (HiddenUsersManager::Instance().isHidden(user->id)) {
				++hidden;
			}
		}
	} else if (const auto channel = peer->asChannel()) {
		if (const auto info = channel->mgInfo.get()) {
			for (const auto &user : info->lastParticipants) {
				if (HiddenUsersManager::Instance().isHidden(user->id)) {
					++hidden;
				}
			}
		}
	}
	return std::max(rawCount - hidden, 0);
}

} // namespace HiddenUsers
