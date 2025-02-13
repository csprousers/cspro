#pragma once

class DataRepository;


//Get a syncable repository from a dictionary name.

struct IDataRepositoryRetriever
{
    virtual ~IDataRepositoryRetriever() { }

    virtual DataRepository* get(const std::wstring& dictionary_name) = 0;
};
