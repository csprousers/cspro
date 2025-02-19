#pragma once

#include <zMessageO/zMessageO.h>

class MessageFile;
class TextSource;


class ZMESSAGEO_API SystemMessages
{
public:
    // Loads the system message files along with any additional message files that should be considered system messages.
    static void LoadMessages(const std::string& application_file_path,
                             const std::vector<std::shared_ptr<const TextSource>>& additional_message_text_sources);

    // Gets the system messages file. If it has not been loaded, it will automatically be loaded.
    static MessageFile& GetMessageFile();
    static std::shared_ptr<MessageFile> GetSharedMessageFile();

    // Sets the system messages file.
    static void SetMessageFile(std::shared_ptr<MessageFile> message_file);

    // Returns whether or not the current system messages file contains any customized messages.
    static bool ApplicationUsesCustomMessages();
};
