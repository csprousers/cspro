#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

namespace FileIO { class TextFile; }


class ZENGINEO_API LogicFile : public Symbol
{
private:
    LogicFile(const LogicFile& logic_file);

public:
    LogicFile(std::string file_name);
    ~LogicFile() noexcept;

    bool IsUsed() const noexcept { return m_isUsed; }
    void SetUsed()      noexcept { m_isUsed = true; }

    bool HasGlobalVisibility() const noexcept { return m_hasGlobalVisibility; }
    void SetGlobalVisibility()       noexcept { m_hasGlobalVisibility = true; }

    bool IsWrittenTo() const noexcept { return m_isWrittenTo; }
    void SetIsWrittenTo()    noexcept { m_isWrittenTo = true; }

    const std::string& GetFilePath() const  noexcept { return m_filePath; }
    void SetFilePath(std::string file_path) noexcept { m_filePath = std::move(file_path); }

    bool IsOpen() const noexcept { return ( m_textFile != nullptr ); }

    FileIO::TextFile& GetTextFile() noexcept { ASSERT(IsOpen()); return *m_textFile; }

    // the runtime actions Open, Close, and StartOperation can throw exceptions
    void Open(bool create, bool append, bool create_if_not_exist);

    void Close();

    // Calls a file repositioning function (as required when switching from reading <-> writing).
    void StartOperation(bool writing);

    // Symbol overrides
    void CopyCompileTimeAttributes(const Symbol& symbol) override;

    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteValueToJson(JsonWriter& json_writer) const override;

private:
    void Close_noexcept() noexcept;

private:
    bool m_isUsed;
    bool m_hasGlobalVisibility;
    bool m_isWrittenTo;
    std::string m_filePath;
    std::unique_ptr<FileIO::TextFile> m_textFile;
    std::optional<bool> m_lastOperationWasWriting;
};
