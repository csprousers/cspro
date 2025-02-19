#pragma once

#include <zUtilO/ApplicationLoadException.h>

class Application;
class CDataDict;
class CDEFormFile;
class CTabSet;
class MessageManager;
class SystemMessageIssuer;


class ApplicationLoader
{
public:
    virtual ~ApplicationLoader() { }

    // Returns a non-null pointer to the application.
    virtual Application* GetApplication() = 0;

    virtual std::shared_ptr<CDataDict> GetDictionary(const std::string& dictionary_file_path) = 0;

    virtual std::shared_ptr<CDEFormFile> GetFormFile(const std::string& form_file_path) = 0;

    virtual std::shared_ptr<CTabSet> GetTableSpec(const std::string& table_spec_file_path) = 0;

    virtual std::shared_ptr<SystemMessageIssuer> GetSystemMessageIssuer() { return nullptr; }

    virtual std::shared_ptr<MessageManager> GetSystemMessages() = 0;
    virtual std::shared_ptr<MessageManager> GetUserMessages() = 0;

    virtual void ProcessUserMessagesPostCompile(MessageManager& /*user_message_manager*/) { }

    virtual void ProcessResources() { }
};


/* the order that methods are called: APP_LOAD_TODO = [ALT]

    application object              [do for non-batch]
    external dictionaries           [do for non-batch]
    dictionaries + forms            [do for non-batch]
    - GetSystemMessages
    - GetUserMessages
    [ALT] logic
    [ALT] write out compiled logic
    - ProcessUserMessagesPostCompile (which, for the .pen serialization, should write out the user messages)
    [ALT] capi
    - ProcessResources
*/
