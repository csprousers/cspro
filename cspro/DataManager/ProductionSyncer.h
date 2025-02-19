#pragma once

#include <zAppO/PFF.h>
#include <zSyncO/SyncRunner.h>


class ProductionSyncer
{
private:
    ProductionSyncer(std::unique_ptr<const PFF> pff);

public:
    // Instantiates a ProductionSyncer only when the command line arguments call for it.
    // Exceptions are thrown if the command line arguments are invalid.
    static std::unique_ptr<ProductionSyncer> Create(const std::vector<std::wstring>& file_paths);

    // Runs the syncs, with errors shown. The only exception thrown will be UserCanceledException.
    void RunSyncs();

    std::shared_ptr<const PFF> GetPff() const { return m_pff; }

    const std::vector<ConnectionString>& GetConnectionStringsForSyncedDataSources() { return m_connectionStringsForSyncedDataSources; }

private:
    void RunSync(SyncRunner::Connection sync_runner_connection, const std::string& dictionary_name, const ConnectionString& connection_string);

private:
    std::shared_ptr<const PFF> m_pff;
    std::vector<ConnectionString> m_connectionStringsForSyncedDataSources;
};
