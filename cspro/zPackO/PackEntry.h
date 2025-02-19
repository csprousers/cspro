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
    bool resources = false;
    bool pff = true;
};


class ZPACKO_API PackEntry
{
public:
    // creates a PackEntry or subclass based on the path
    static std::unique_ptr<PackEntry> Create(std::string path);

    PackEntry(std::string path, bool entry_is_file);
    virtual ~PackEntry() { }

    const std::string& GetPath() const { return m_path; }

    // returns the various extras (if applicable)
    virtual DirectoryPackEntryExtras* GetDirectoryExtras()     { return nullptr; }
    const DirectoryPackEntryExtras* GetDirectoryExtras() const { return const_cast<PackEntry*>(this)->GetDirectoryExtras(); }

    virtual DictionaryPackEntryExtras* GetDictionaryExtras()     { return nullptr; }
    const DictionaryPackEntryExtras* GetDictionaryExtras() const { return const_cast<PackEntry*>(this)->GetDictionaryExtras(); }

    virtual PffPackEntryExtras* GetPffExtras()     { return nullptr; }
    const PffPackEntryExtras* GetPffExtras() const { return const_cast<PackEntry*>(this)->GetPffExtras(); }

    virtual ApplicationPackEntryExtras* GetApplicationExtras()     { return nullptr; }
    const ApplicationPackEntryExtras* GetApplicationExtras() const { return const_cast<PackEntry*>(this)->GetApplicationExtras(); }

    // returns all files associated with the entry (including m_path, if applicable);
    // the same path may occur more than once in the list;
    // the path is not guaranteed to exist on the disk
    virtual std::vector<std::string> GetAssociatedFilePaths() const { return { m_path }; }

    // returns a list of files (taken from the virtual GetAssociatedFilePaths method)
    // that can be used to show what files are included as part of this entry;
    // the first string is the full path and the second string is the filename for displaying
    std::vector<std::tuple<std::string, std::string>> GetFilenamesForDisplay() const;

protected:
    std::string m_path;
};


class DirectoryPackEntry : public PackEntry
{
public:
    DirectoryPackEntry(std::string path);

    DirectoryPackEntryExtras* GetDirectoryExtras() override;
    std::vector<std::string> GetAssociatedFilePaths() const override;

private:
    DirectoryPackEntryExtras m_directoryExtras;

    mutable std::map<bool, std::vector<std::string>> m_directoryFilePaths;
};


class DictionaryPackEntry : public PackEntry
{
public:
    DictionaryPackEntry(std::string path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras);

    DictionaryPackEntryExtras* GetDictionaryExtras() override;
    std::vector<std::string> GetAssociatedFilePaths() const override;

private:
    std::shared_ptr<DictionaryPackEntryExtras> m_dictionaryExtras;

    mutable std::unique_ptr<std::vector<std::string>> m_valueSetImageFilePaths;
};


class FormPackEntry : public PackEntry
{
public:
    FormPackEntry(std::string path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras);

    DictionaryPackEntryExtras* GetDictionaryExtras() override;
    std::vector<std::string> GetAssociatedFilePaths() const override;

private:
    std::unique_ptr<DictionaryPackEntry> m_dictionaryPackEntry;
};


class TabSpecPackEntry : public PackEntry
{
public:
    TabSpecPackEntry(std::string path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras);

    DictionaryPackEntryExtras* GetDictionaryExtras() override;
    std::vector<std::string> GetAssociatedFilePaths() const override;

private:
    std::unique_ptr<DictionaryPackEntry> m_dictionaryPackEntry;
};


class PffPackEntry : public PackEntry
{
public:
    PffPackEntry(std::string path, std::shared_ptr<PffPackEntryExtras> pff_extras);
    ~PffPackEntry();

    PffPackEntryExtras* GetPffExtras() override;
    std::vector<std::string> GetAssociatedFilePaths() const override;

private:
    std::shared_ptr<PffPackEntryExtras> m_pffExtras;

    mutable std::unique_ptr<PFF> m_pff;
    mutable std::unique_ptr<std::vector<std::string>> m_inputDataFilePaths;
    mutable std::unique_ptr<std::vector<std::string>> m_externalDictionaryDataFilePaths;
    mutable std::unique_ptr<std::vector<std::string>> m_userFilePaths;
};


class ApplicationPackEntry : public PackEntry
{
public:
    ApplicationPackEntry(std::string path);
    ApplicationPackEntry(ApplicationPackEntry&& rhs) = default;

    DictionaryPackEntryExtras* GetDictionaryExtras() override;
    PffPackEntryExtras* GetPffExtras() override;
    ApplicationPackEntryExtras* GetApplicationExtras() override;
    std::vector<std::string> GetAssociatedFilePaths() const override;

private:
    std::shared_ptr<DictionaryPackEntryExtras> m_dictionaryExtras;
    ApplicationPackEntryExtras m_applicationExtras;

    std::vector<std::string> m_applicationFilePaths;
    std::vector<FormPackEntry> m_formPackEntries;
    std::vector<TabSpecPackEntry> m_tabSpecPackEntries;
    std::vector<DictionaryPackEntry> m_externalDictionaryPackEntries;
    mutable std::vector<std::tuple<AppResource, std::unique_ptr<std::vector<std::string>>>> m_resourcesAndEvaluatedFilePaths;
    std::unique_ptr<PffPackEntry> m_pffPackEntry;
};
