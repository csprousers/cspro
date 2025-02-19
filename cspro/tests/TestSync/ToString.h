#pragma once

#include <zSyncO/SyncClient.h>


namespace Microsoft::VisualStudio::CppUnitTestFramework
{
    // CString
    template<>
    inline std::wstring ToString<CString>(const CString& text)
    {
        return std::wstring(text.GetString(), text.GetLength());
    }


    // std::vector<std::string>
    template<>
    inline std::wstring ToString<std::vector<std::string>>(const std::vector<std::string>& values)
    {
        return TC::ToWide(SO::CreateSingleString(values));
    }


    // SyncClient::SyncResult
    template<>
    inline std::wstring ToString<SyncClient::SyncResult>(const SyncClient::SyncResult& result)
    {
        switch( result )
        {
            case SyncClient::SyncResult::SYNC_OK:       return L"SyncResult::SYNC_OK";
            case SyncClient::SyncResult::SYNC_ERROR:    return L"SyncResult::SYNC_ERROR";
            case SyncClient::SyncResult::SYNC_CANCELED: return L"SyncResult::SYNC_CANCELED";
            default:                                    return L"THIS SHOULD NEVER HAPPEN";
        }        
    }


    // SyncPutResponse::SyncGetResult
    template<>
    inline std::wstring ToString<SyncGetResponse::SyncGetResult>(const SyncGetResponse::SyncGetResult& result)
    {
        switch( result )
        {
            case SyncGetResponse::SyncGetResult::Complete:         return L"SyncGetResult::Complete";
            case SyncGetResponse::SyncGetResult::MoreData:         return L"SyncGetResult::MoreData";
            case SyncGetResponse::SyncGetResult::RevisionNotFound: return L"SyncGetResult::RevisionNotFound";
            default:                                               return L"THIS SHOULD NEVER HAPPEN";
        }        
    }


    // SyncPutResponse::SyncPutResult
    template<>
    inline std::wstring ToString<SyncPutResponse::SyncPutResult>(const SyncPutResponse::SyncPutResult& result)
    {
        switch( result )
        {
            case SyncPutResponse::SyncPutResult::Complete:         return L"SyncPutResult::Complete";
            case SyncPutResponse::SyncPutResult::RevisionNotFound: return L"SyncPutResult::RevisionNotFound";
            default:                                               return L"THIS SHOULD NEVER HAPPEN";
        }
    }


    // PartialSaveMode
    template<>
    inline std::wstring ToString<PartialSaveMode>(const PartialSaveMode& mode)
    {
        switch( mode )
        {
            case PartialSaveMode::None:   return L"PartialSaveMode::None";
            case PartialSaveMode::Add:    return L"PartialSaveMode::Add";
            case PartialSaveMode::Modify: return L"PartialSaveMode::Modify";
            case PartialSaveMode::Verify: return L"PartialSaveMode::Verify";
            default:                      return L"THIS SHOULD NEVER HAPPEN";
        }
    }
}
