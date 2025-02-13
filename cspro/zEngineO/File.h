#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

enum class Encoding : int;


class ZENGINEO_API LogicFile : public Symbol
{
private:
    LogicFile(const LogicFile& logic_file);

public:
    LogicFile(std::wstring file_name);
    ~LogicFile();

    bool IsUsed() const { return m_isUsed; }
    void SetUsed()      { m_isUsed = true; }

    bool HasGlobalVisibility() const { return m_hasGlobalVisibility; }
    void SetGlobalVisibility()       { m_hasGlobalVisibility = true; }

    bool IsWrittenTo() const { return m_isWrittenTo; }
    void SetIsWrittenTo()    { m_isWrittenTo = true; }

    const std::wstring& GetFilename() const { return m_filename; }
    void SetFilename(std::wstring filename) { m_filename = std::move(filename); }

    bool Open(bool create_new, bool append, bool truncate);
    bool IsOpen() const;

    bool Close();

    CFile& GetFile() { return m_file; }

    Encoding GetEncoding() const { return m_encoding; }

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteValueToJson(JsonWriter& json_writer) const override;

private:
    bool m_isUsed;
    bool m_hasGlobalVisibility;
    bool m_isWrittenTo;
    std::wstring m_filename;
    CFile m_file;
    Encoding m_encoding;
};
