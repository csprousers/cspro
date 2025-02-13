#pragma once

#include <zPackO/zPackO.h>

class PFF;


struct DirectoryPackEntryExtras
{
    bool recursive = true;
};

struct DictionaryPackEntryExtras
{
    bool value_set_images = false;
};

struct PffPackEntryExtras
{
    bool input_data = false;
    bool external_dictionary_data = false;
    bool user_files = false;
};

struct ApplicationPackEntryExtras
{
    bool resource_folders = false;
    bool pff = true;
};


class ZPACKO_API PackEntry
{
public:
    // creates a PackEntry or subclass based on the path
    static std::unique_ptr<PackEntry> Create(std::wstring path);

    PackEntry(std::wstring path, bool entry_is_file);
    virtual ~PackEntry() { }

    const std::wstring& GetPath() const { return m_path; }

    // returns the various extras (if applicable)
    virtual DirectoryPackEntryExtras* GetDirectoryExtras()     { return nullptr; }
    const DirectoryPackEntryExtras* GetDirectoryExtras() const { return const_cast<PackEntry*>(this)->GetDirectoryExtras(); }

    virtual DictionaryPackEntryExtras* GetDictionaryExtras()     { return nullptr; }
    const DictionaryPackEntryExtras* GetDictionaryExtras() const { return const_cast<PackEntry*>(this)->GetDictionaryExtras(); }

    virtual PffPackEntryExtras* GetPffExtras()     { return nullptr; }
    const PffPackEntryExtras* GetPffExtras() const { return const_cast<PackEntry*>(this)->GetPffExtras(); }

    virtual ApplicationPackEntryExtras* GetApplicationExtras()     { return nullptr; }
    const ApplicationPackEntryExtras* GetApplicationExtras() const { return const_cast<PackEntry*>(this)->GetApplicationExtras(); }

    // returns all files associated with the entry (including the filename);
    // the same filename may occur more than once in the list;
    // the filename is not guaranteed to exist on the disk
    virtual std::vector<std::wstring> GetAssociatedFilenames() const { return { m_path }; }

    // returns a list of files (taken from the virtual GetAssociatedFilenames method)
    // that can be used to show what files are included as part of this entry;
    // the first string is the full path and the second string is the filename for displaying
    std::vector<std::tuple<std::wstring, std::wstring>> GetFilenamesForDisplay() const;

protected:
    const std::wstring m_path;
};


class DirectoryPackEntry : public PackEntry
{
public:
    DirectoryPackEntry(std::wstring path);

    DirectoryPackEntryExtras* GetDirectoryExtras() override;
    std::vector<std::wstring> GetAssociatedFilenames() const override;

private:
    DirectoryPackEntryExtras m_directoryExtras;

    mutable std::map<bool, std::vector<std::wstring>> m_directoryFilenames;
};


class DictionaryPackEntry : public PackEntry
{
public:
    DictionaryPackEntry(std::wstring path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras);

    DictionaryPackEntryExtras* GetDictionaryExtras() override;
    std::vector<std::wstring> GetAssociatedFilenames() const override;

private:
    std::shared_ptr<DictionaryPackEntryExtras> m_dictionaryExtras;

    mutable std::unique_ptr<std::vector<std::wstring>> m_valueSetImageFilenames;
};


class FormPackEntry : public PackEntry
{
public:
    FormPackEntry(std::wstring path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras);

    DictionaryPackEntryExtras* GetDictionaryExtras() override;
    std::vector<std::wstring> GetAssociatedFilenames() const override;

private:
    std::unique_ptr<DictionaryPackEntry> m_dictionaryPackEntry;
};


class TabSpecPackEntry : public PackEntry
{
public:
    TabSpecPackEntry(std::wstring path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras);

    DictionaryPackEntryExtras* GetDictionaryExtras() override;
    std::vector<std::wstring> GetAssociatedFilenames() const override;

private:
    std::unique_ptr<DictionaryPackEntry> m_dictionaryPackEntry;
};


class PffPackEntry : public PackEntry
{
public:
    PffPackEntry(std::wstring path, std::shared_ptr<PffPackEntryExtras> pff_extras);
    ~PffPackEntry();

    PffPackEntryExtras* GetPffExtras() override;
    std::vector<std::wstring> GetAssociatedFilenames() const override;

private:
    std::shared_ptr<PffPackEntryExtras> m_pffExtras;

    mutable std::unique_ptr<PFF> m_pff;
    mutable std::unique_ptr<std::vector<std::wstring>> m_inputDataFilenames;
    mutable std::unique_ptr<std::vector<std::wstring>> m_externalDictionaryDataFilenames;
    mutable std::unique_ptr<std::vector<std::wstring>> m_userFilenames;
};


class ApplicationPackEntry : public PackEntry
{
public:
    ApplicationPackEntry(std::wstring path);
    ApplicationPackEntry(ApplicationPackEntry&& rhs) = default;

    DictionaryPackEntryExtras* GetDictionaryExtras() override;
    PffPackEntryExtras* GetPffExtras() override;
    ApplicationPackEntryExtras* GetApplicationExtras() override;
    std::vector<std::wstring> GetAssociatedFilenames() const override;

private:
    std::shared_ptr<DictionaryPackEntryExtras> m_dictionaryExtras;
    ApplicationPackEntryExtras m_applicationExtras;

    std::vector<std::wstring> m_applicationFilenames;
    std::vector<FormPackEntry> m_formPackEntries;
    std::vector<TabSpecPackEntry> m_tabSpecPackEntries;
    std::vector<DictionaryPackEntry> m_externalDictionaryPackEntries;
    mutable std::map<std::wstring, std::unique_ptr<std::vector<std::wstring>>> m_resourceFolderFilenames;
    std::unique_ptr<PffPackEntry> m_pffPackEntry;
};
