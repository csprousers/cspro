#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

enum class Encoding : int;


class ZENGINEO_API LogicFile : public Symbol
{
private:
    LogicFile(const LogicFile& logic_file);

public:
    LogicFile(std::string file_name);
    ~LogicFile();

    bool IsUsed() const { return m_isUsed; }
    void SetUsed()      { m_isUsed = true; }

    bool HasGlobalVisibility() const { return m_hasGlobalVisibility; }
    void SetGlobalVisibility()       { m_hasGlobalVisibility = true; }

    bool IsWrittenTo() const { return m_isWrittenTo; }
    void SetIsWrittenTo()    { m_isWrittenTo = true; }

    const std::string& GetFilePath() const  { return m_filePath; }
    void SetFilePath(std::string file_path) { m_filePath = std::move(file_path); }

    bool Open(bool create_new, bool append, bool truncate);
    bool IsOpen() const;

    bool Close();

    CFile& GetFile() { return m_file; }

    Encoding GetEncoding() const { return m_encoding; }

    // Symbol overrides
    void CopyCompileTimeAttributes(const Symbol& symbol) override;

    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteValueToJson(JsonWriter& json_writer) const override;

private:
    bool m_isUsed;
    bool m_hasGlobalVisibility;
    bool m_isWrittenTo;
    std::string m_filePath;
    CFile m_file;
    Encoding m_encoding;
};
