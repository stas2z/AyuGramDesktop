#pragma once

#include "data/data_peer_id.h"
#include <QSet>
#include <QString>

class HiddenUsersManager {
public:
    static HiddenUsersManager &Instance();

    void loadFromFile();
    bool isHidden(PeerId peerId) const;

private:
    HiddenUsersManager();

    QSet<long long> _hiddenUserIds;
};
