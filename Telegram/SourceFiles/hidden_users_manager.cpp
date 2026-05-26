// HiddenUsersManager.cpp

#include "hidden_users_manager.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QSet>

namespace {

QStringList GetCandidatePaths() {
    const auto appName = QCoreApplication::applicationName();
    QStringList paths;

    // 1. Директория запуска (для portable версии или разработки)
    paths.push_back(QCoreApplication::applicationDirPath() + "/hidden_users.txt");

    // 2. Стандартные пути данных Telegram Desktop
    // Linux
    paths.push_back(QDir::homePath() + "/.local/share/TelegramDesktop/hidden_users.txt");
    // macOS
    paths.push_back(QDir::homePath() + "/Library/Application Support/Telegram Desktop/hidden_users.txt");
    // Windows
    paths.push_back(QDir::homePath() + "/AppData/Roaming/Telegram Desktop/hidden_users.txt");

    return paths;
}

} // namespace

HiddenUsersManager &HiddenUsersManager::Instance() {
    static HiddenUsersManager instance;
    return instance;
}

HiddenUsersManager::HiddenUsersManager() {
    loadFromFile();
}

void HiddenUsersManager::loadFromFile() {
    _hiddenUserIds.clear();
    
    auto paths = GetCandidatePaths();
    QString loadedPath;

    for (const auto &path : paths) {
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            loadedPath = path;
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                // Пропускаем комментарии и пустые строки
                if (line.isEmpty() || line.startsWith('#') || line.startsWith("//")) {
                    continue;
                }
                
                bool ok = false;
                long long id = line.toLongLong(&ok);
                if (ok && id != 0) {
                    _hiddenUserIds.insert(id);
                }
            }
            file.close();
            break; // Загружаем из первого найденного файла
        }
    }

    // Debug logging
    if (!loadedPath.isEmpty()) {
        qDebug() << "[HiddenUsers] Loaded from:" << loadedPath;
        qDebug() << "[HiddenUsers] Count:" << _hiddenUserIds.size();
        // Выведем первые 5 ID для проверки (в реальном релизе лучше убрать или сделать условным)
        auto list = _hiddenUserIds.values();
        for(int i = 0; i < std::min(5, list.size()); ++i) {
            qDebug() << "[HiddenUsers] ID:" << list[i];
        }
    } else {
        qDebug() << "[HiddenUsers] File not found in any candidate path.";
    }
}

bool HiddenUsersManager::isHidden(PeerId peerId) const {
    if (peerId.peerType != PeerIdType::User) {
        return false;
    }
    // PeerId хранит полный ID, нам нужно извлечь bare ID пользователя
    // Обычно peerId.bare() возвращает нужный нам идентификатор
    const auto userId = peerId.bare();
    
    bool result = _hiddenUserIds.contains(userId);
    if (result) {
        // qDebug() << "[HiddenUsers] Hiding user:" << userId;
    }
    return result;
}
