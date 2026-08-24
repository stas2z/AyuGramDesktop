# Hidden Users Feature

*[Русская версия](HIDDEN_USERS_README.ru.md)*

## Description
Implements hiding specific users from Telegram Desktop's visualization lists, based on a list of user IDs.

## How it works

### 1. Reading the user ID list from a file
- File: `hidden_users.txt`
- Location:
  - First looked up in the application's current working directory
  - If not found, falls back to the application data directory (AppDataLocation)
- Format: one user ID per line (numeric)
- Comments: lines starting with `#` or `//` are ignored

### 2. Excluded from visualization
Hidden users are excluded from the following lists:
- Group/channel participant lists (`edit_participants_box.cpp`)
- Chat lists (`peer_list_controllers.cpp` - ChatsListBoxController)
- Contact lists (`peer_list_controllers.cpp` - ContactsBoxController)
- Their messages in the open chat itself (`history_view_list_widget.cpp` - `ListWidget::refreshRows`)
- Their messages in global and in-chat message search results (`dialogs_inner_widget.cpp` - `InnerWidget::searchReceived`)

### 3. Searching by @username
- Searching by @username (peer/contact search) **still finds** hidden users
- This lets you open a chat with a hidden user when needed
- Peer-search filtering happens only in `appendRow()` / `prependRow()`, not in `createSearchRow()`
- **Message** search (Ctrl+F, global search) does filter out messages from hidden users - they don't show up in results

### 4. Mentions (@mention) in message text
- A hidden user's `@username` mention is masked right in the text: the username characters are replaced with `•` (length preserved, so other entities' offsets don't shift), plus the link/highlight is stripped
  - Masking is necessary - otherwise the username stays readable as plain text and can be typed into search to find the hidden user
- Text mentions without a username (`MentionName`, a clickable name shown without `@`) of a hidden user only lose their link/highlight - the name text itself isn't masked, since it doesn't contain a searchable username
- Implemented in `HistoryItem::translatedTextWithLocalEntities()` (`history_item.cpp`): `@username` is resolved to a peer via `peerByUsername()`, `MentionName` directly via the `userId` encoded in the entity

### 5. Notifications
- Messages from hidden users don't trigger a desktop notification (name + text) - the check lives in `Window::Notifications::System::schedule()` (`notifications_manager.cpp`), next to the existing `isMessageHidden()` check
- Checks the message sender (`item->from()`), not the thread's peer - works both for direct chats and for a hidden user's messages inside a group

### 6. Logging
All actions are logged to the console with the `[HiddenUsers]` prefix:
- Loading the ID list file
- Number of IDs loaded
- Skipped invalid lines
- Every time a hidden user is skipped from a list

## Example hidden_users.txt

```
# List of hidden users
# One ID per line

# Examples:
123456789
987654321
# This is a comment
// This is also a comment
```

## Technical details

### Changed files:
1. `Telegram/SourceFiles/hidden_users_manager.h` - manager header
2. `Telegram/SourceFiles/hidden_users_manager.cpp` - manager implementation
3. `Telegram/SourceFiles/boxes/peers/edit_participants_box.cpp` - hiding in participant lists
4. `Telegram/SourceFiles/boxes/peer_list_controllers.cpp` - hiding in chat and contact lists
5. `Telegram/SourceFiles/history/view/history_view_list_widget.cpp` - hiding messages in the open chat (`ListWidget::refreshRows`)
6. `Telegram/SourceFiles/dialogs/dialogs_inner_widget.cpp` - hiding messages in search results (`InnerWidget::searchReceived`)
7. `Telegram/SourceFiles/history/history_item.cpp` - stripping the link from hidden users' mentions (`translatedTextWithLocalEntities`)
8. `Telegram/SourceFiles/window/notifications_manager.cpp` - suppressing desktop notifications (`System::schedule`)
9. `Telegram/CMakeLists.txt` - added the manager files to the build

### Initialization
The hidden user list is loaded automatically when a session is created, in `main_session.cpp` (line 308):
```cpp
HiddenUsersManager::Instance().loadFromFile();
```

### API for checking
```cpp
if (HiddenUsersManager::Instance().isHidden(peerId)) {
    // User is hidden
}
```

## Notes
- Hiding only works for users (PeerIdType::User)
- Groups, channels, and other peer types are never hidden
- User IDs must be in numeric format (bare ID, no prefixes)
- `hidden_users.txt` is watched via `QFileSystemWatcher` - changes are picked up live (`HiddenUsersManager::watchFile`), and the ID list is reloaded from scratch (including IDs removed from the file); already-open lists/chats will refresh on their next natural rebuild, there's no forced instant redraw
- All hiding happens purely at the display (UI) layer: message data, unread counts, and history still load normally - nothing is deleted or blocked at the network/data level
