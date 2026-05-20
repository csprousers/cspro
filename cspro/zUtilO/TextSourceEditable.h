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

    // If the file does not exist or can not be read, the default text will be used.
    // If no default text is provided, exceptions will be thrown.
    TextSourceEditable(std::string file_path, std::optional<std::string> default_text = std::nullopt,
                       bool use_default_text_even_if_file_exists = false);

    // Sends a message to the Designer to locate an already-open instance of this file.
    // If none are open, the file is opened.
    static std::shared_ptr<TextSourceEditable> FindOpenOrCreate(std::string file_path);

    // Reloads the file from the disk.
    const std::string& ReloadFromDisk();

    // Returns the modified time of the file that was loaded or saved by this object.
    int64_t GetLoadedFileModifiedTime() const { return m_loadedFileModifiedTime; }

    // Returns true if the file on the disk is different from what was loaded.
    bool FileOnDiskHasChanged() const;

    const std::string& GetText() const override;
    SharableString GetTextAsSharableString() const override;

    int64_t GetModifiedIteration() const override { return m_modifiedIteration; }

    void SetText(SharableString text) override;

    bool RequiresSave() const override { return m_modified; }

    void SetModified();

    void Save() override;

    void SetNewFilePath(std::string new_file_path);

    void SetSourceModifier(SourceModifier* source_modifier);

private:
    void SyncText() const;

private:
    SharableString m_text;

    bool m_modified;
    int64_t m_loadedFileModifiedTime;
    int64_t m_modifiedIteration;

    SourceModifier* m_sourceModifier;
    mutable int64_t m_sourceModifierLastGetTextModifiedIteration;
};
