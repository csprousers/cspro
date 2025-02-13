#pragma once

#include <zMessageO/zMessageO.h>

class MessageFile;
class TextSource;


class ZMESSAGEO_API SystemMessages
{
public:
    // Returns whether or not a message filename is considered a system message file.
    static bool IsMessageFilenameSystemMessages(const std::wstring& message_filename);

    // Loads the system message files with arguments that specify additional message files to load.
    static void LoadMessages(const std::wstring& application_filename, const std::vector<std::shared_ptr<TextSource>>& application_message_text_sources,
                             bool load_designer_messages);

    // Gets the system messages file. If it has not been loaded, it will automatically be loaded.
    static MessageFile& GetMessageFile();
    static std::shared_ptr<MessageFile> GetSharedMessageFile();

    // Sets the system messages file.
    static void SetMessageFile(std::shared_ptr<MessageFile> message_file);

    // Returns whether or not the current system messages file contains any customized messages.
    static bool ApplicationUsesCustomMessages();
};
