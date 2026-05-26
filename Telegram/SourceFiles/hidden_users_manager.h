#pragma once

#include "base/unique_func.h"
#include "data/peer_id.h"
#include <QSet>

class HiddenUsersManager {
public:
    static HiddenUsersManager &Instance();

    void loadFromFile();
    bool isHidden(PeerId peerId) const;

private:
    HiddenUsersManager();

    QSet<long long> _hiddenUserIds;
};
