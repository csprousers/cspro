#pragma once


struct DataSyncStatistics
{
    size_t cases_not_in_repository = 0;
    size_t cases_newer_in_repository = 0;
    size_t cases_newer_on_remote = 0;
    size_t cases_with_conflicts = 0;
    size_t cases_received = 0;
    size_t cases_sent = 0;

    DataSyncStatistics& operator+=(const DataSyncStatistics& rhs)
    {
        cases_not_in_repository += rhs.cases_not_in_repository;
        cases_newer_in_repository += rhs.cases_newer_in_repository;
        cases_newer_on_remote += rhs.cases_newer_on_remote;
        cases_with_conflicts += rhs.cases_with_conflicts;
        cases_received += rhs.cases_received;
        cases_sent += rhs.cases_sent;
        return *this;
    }
};
