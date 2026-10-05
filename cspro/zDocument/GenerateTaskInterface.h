#pragma once

class GlobalSettings;


enum class GenerateTaskStatus { NotStarted, Running, Complete, Canceled, EndedInException };


class GenerateTaskInterface
{
public:
    virtual ~GenerateTaskInterface() noexcept = default;

    virtual void SetTitle(const std::string& title) = 0;

    virtual void LogText(SharableString text) = 0;

    template<typename... Args>
    void LogText(const char* formatter, Args const&... args);

    virtual void UpdateProgress(double percent) = 0;

    virtual void SetOutputText(const std::string& text) = 0;

    virtual void OnCreatedOutput(std::string output_title, std::string path) = 0;

    virtual void OnException(const CSProException& exception) = 0;

    virtual void OnCompletion(GenerateTaskStatus status) = 0;

    virtual const GlobalSettings& GetGlobalSettings() = 0;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
void GenerateTaskInterface::LogText(const char* const formatter, Args const&... args)
{
    LogText(FormatText(formatter, args...));
}
