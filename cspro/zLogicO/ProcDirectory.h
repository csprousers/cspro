#pragma once


namespace Logic
{
    struct ProcDirectoryEntry
    {
        size_t first_basic_token_index;
        size_t number_basic_tokens;
    };


    class ProcDirectory
    {
    public:
        void AddEntry(int symbol_index, size_t first_basic_token_index, size_t number_basic_tokens)
        {
            ASSERT(GetEntry(symbol_index) == nullptr);

            m_entries.try_emplace(symbol_index, ProcDirectoryEntry { first_basic_token_index, number_basic_tokens });
        }

        const ProcDirectoryEntry* GetEntry(int symbol_index) const
        {
            const auto& map_find = m_entries.find(symbol_index);
            return ( map_find != m_entries.end() ) ? &map_find->second : nullptr;
        }

        const std::map<int, ProcDirectoryEntry>& GetEntries() const
        {
            return m_entries;
        }

    private:
        std::map<int, ProcDirectoryEntry> m_entries;
    };
}
