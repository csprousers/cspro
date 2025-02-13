#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/TextSource.h>


class CLASS_DECL_ZUTILO TextSourceEditable : public TextSource
{
public:
    class SourceModifier
    {
    public:
        virtual ~SourceModifier() { }
        virtual void SyncTextSource() = 0;
        virtual void OnTextSourceSave() = 0;
    };

    // if the file does not exist or can not be read, the default text will be used;
    // if no default text is provided, exceptions will be thrown
    TextSourceEditable(std::wstring filename, std::optional<std::wstring> default_text = std::nullopt,
                       bool use_default_text_even_if_file_exists = false);

    // sends a message to the Designer to locate an already-open instance of this file;
    // if none are open, the file is opened
    static std::shared_ptr<TextSourceEditable> FindOpenOrCreate(std::wstring filename, std::optional<std::wstring> default_text = std::nullopt);

    const std::wstring& ReloadFromDisk();

    const std::wstring& GetText() const override;

    int64_t GetModifiedIteration() const override { return m_modifiedIteration; }

    void SetText(std::wstring text) override;

    bool RequiresSave() const override { return m_modified; }

    void SetModified();

    void Save() override;

    void SetNewFilename(std::wstring new_filename);

    void SetSourceModifier(SourceModifier* source_modifier);

private:
    std::wstring m_text;
    bool m_modified;
    int64_t m_modifiedIteration;
    SourceModifier* m_sourceModifier;
    mutable int64_t m_sourceModifierLastGetTextModifiedIteration;
};
