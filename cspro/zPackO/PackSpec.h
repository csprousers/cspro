#pragma once

#include <zPackO/zPackO.h>
#include <zPackO/PackEntry.h>
#include <zToolsO/SharedPointerHelpers.h>

class PFF;
namespace JsonSpecFile { class ReaderMessageLogger; }


class ZPACKO_API PackSpec
{
public:
    static constexpr double VersionNewPackIntroduced = 8.0;

    const std::string& GetZipFilePath() const      { return m_zipFilePath; }
    void SetZipFilePath(std::string zip_file_path) { m_zipFilePath = std::move(zip_file_path); }

    size_t GetNumEntries() const { return m_packEntries.size(); }

    const SharedPointerVectorWrapper<PackEntry> GetEntries() const { return SharedPointerVectorWrapper<PackEntry>(m_packEntries); }

    const PackEntry& GetEntry(size_t index) const { ASSERT(index < m_packEntries.size()); return *m_packEntries[index]; }
    PackEntry& GetEntry(size_t index)             { ASSERT(index < m_packEntries.size()); return *m_packEntries[index]; }

    void AddEntry(std::shared_ptr<PackEntry> pack_entry) { m_packEntries.emplace_back(std::move(pack_entry)); }
    void RemoveEntry(size_t index)                       { ASSERT(index < m_packEntries.size()); m_packEntries.erase(m_packEntries.begin() + index); }
    void RemoveAllEntries()                              { m_packEntries.clear(); }

    std::vector<std::string> GetFilePathsForPack() const;

    // returns true if the PFF's application filename points to a pack specification file
    static bool IsPffUsingPackSpec(const PFF& pff);

    // creates a pack specification from the PFF; extra files defined in the PFF are not included
    static PackSpec CreateFromPff(const PFF& pff, bool silent, bool throw_exception_on_missing_entry);

    // Load and Save throw exceptions
    void Load(const std::string& file_path, bool silent, bool throw_exception_on_missing_entry);
    void Load(const JsonNode& json_node, bool throw_exception_on_missing_entry);
    void Save(const std::string& file_path) const;

private:
    std::string m_zipFilePath;
    SharedPointerAsValueVector<PackEntry> m_packEntries;
};
