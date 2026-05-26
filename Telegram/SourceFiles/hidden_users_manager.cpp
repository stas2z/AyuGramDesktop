// HiddenUsersManager.cpp

#include "hidden_users_manager.h"
#include "ayu/features/hidden_users/ayu_hidden_users.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QSet>
#include <QStandardPaths>

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

void HiddenUsersManager::loadFromFile() {
    const auto filePath = GetFilePath();
    
    qDebug() << "[HiddenUsers] Attempting to load from:" << filePath;
    
    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "[HiddenUsers] File does not exist. Will use empty list.";
        qDebug() << "[HiddenUsers] You can create" << filePath << "with one user ID per line";
        qDebug() << "[HiddenUsers] Lines starting with # or // are treated as comments";
        return;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[HiddenUsers] Failed to open file:" << filePath;
        return;
    }
    
    QTextStream in(&file);
    in.setCodec("UTF-8");
    
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
    const int previewCount = std::min(10, list.size());
    if (previewCount > 0) {
        qDebug() << "[HiddenUsers] First" << previewCount << "IDs:";
        for(int i = 0; i < previewCount; ++i) {
            qDebug() << "  -" << list[i];
        }
    }
}

bool HiddenUsersManager::isHidden(PeerId peerId) const {
    if (peerId.peerType != PeerIdType::User) {
        return false;
    }
    
    // Получаем bare ID пользователя
    const auto userId = peerId.bare();
    
    bool result = _hiddenUserIds.contains(userId);
    if (result) {
        qDebug() << "[HiddenUsers] User is hidden:" << userId;
    }
    return result;
}
