// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <QSet>
#include <QString>
#include <memory>

namespace Main {
class Session;
}

namespace Ayu::HiddenUsers {

// Initialize the hidden users module
void Init(not_null<Main::Session*> session);

// Load hidden user IDs from file
void LoadFromFile(const QString &filePath);

// Check if a user ID should be hidden from lists
[[nodiscard]] bool IsHidden(long long userId);

// Get all hidden user IDs
[[nodiscard]] QSet<long long> GetAllHidden();

// Shutdown the module
void Destroy();

} // namespace Ayu::HiddenUsers
