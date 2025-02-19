#pragma once

#include <DataManager/DataSourceDoc.h>
#include <DataManager/DataSourceFrame.h>


class DataSourceDocTemplate : public CMultiDocTemplate
{
public:
    using CMultiDocTemplate::CMultiDocTemplate;

    CDocTemplate::Confidence MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch) override
    {
        ASSERT(rpDocMatch == nullptr);

        // because a connection string can have various properties that would lead to differing
        // string representations, make sure that this data source is not already open
        std::string path_name = TC::ToUtf8(lpszPathName);
        const ConnectionString connection_string(path_name);
        bool keep_searching_for_data_source = true;

        ForeachDocOfType<DataSourceDoc>(
            [&](DataSourceDoc& data_source_doc)
            {
                if( connection_string.Equals(data_source_doc.GetConnectionString()) )
                {
                    rpDocMatch = &data_source_doc;
                    keep_searching_for_data_source = false;

                    // in case the connection string has parameters to update the
                    // the case that is currently displayed, pass it to the frame for processing
                    WindowsDesktopMessage::PostObject(&data_source_doc.GetFrame(),
                                                      UWM::DataManager::ProcessConnectionStringParameters,
                                                      std::move(path_name));
                }

                return keep_searching_for_data_source;
            });

        return keep_searching_for_data_source ? Confidence::yesAttemptNative :
                                                Confidence::yesAlreadyOpen;
    }
};
