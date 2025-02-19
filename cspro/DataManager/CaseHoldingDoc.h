#pragma once

#include <zDictO/DDClass.h>
#include <zCaseO/Case.h>
#include <zDataO/DataRepository.h>


class CaseHoldingDoc : public CDocument
{
protected:
    CaseHoldingDoc();

public:
    virtual ~CaseHoldingDoc() { }

    const CDataDict& GetDictionary() const                       { return *m_dictionary; }
    std::shared_ptr<const CDataDict> GetSharedDictionary() const { return m_dictionary; }

    const CaseAccess& GetCaseAccess() const                       { return *m_caseAccess; }
    std::shared_ptr<const CaseAccess> GetSharedCaseAccess() const { return m_caseAccess; }

    const ConnectionString& GetConnectionString() const { return m_connectionString; }

    const Case* GetCurrentCase() const                       { return m_currentCase.get(); };
    std::shared_ptr<const Case> GetSharedCurrentCase() const { return m_currentCase; };

    template<typename T>
    std::shared_ptr<T> GetSettings();

    // Returns the directory of the dictionary when the dictionary exists on the disk.
    // If not, the directory of the connection string is returned when the the data source is file-based.
    // If not, a blank string is returned.
    std::string GetDictionaryDirectory() const;

    // Returns the directory of the connection string when the the data source is file-based.
    // If not, the directory of the dictionary is returned when the dictionary exists on the disk.
    // If not, a blank string is returned.
    std::string GetDataDirectory() const;

    // Checks if the dictionary settings allow for exporting data.
    // If not, a message is posted for displaying that indicates why the operation failed.
    bool DictionaryAllowsExport(const char* operation = "this operation") const;

    // Returns true if the dictionary uses binary data.
    bool DictionaryUsesBinaryData() const { return m_dictionaryUsesBinaryData; }

    // methods for subclasses to override
    virtual bool IsDocumentCaseOnly() const = 0;
    virtual UINT GetDefaultPageCommandId() = 0;

protected:
    BOOL RunPostOpenTasks();

protected:
    std::shared_ptr<const CDataDict> m_dictionary;
    std::shared_ptr<const CaseAccess> m_caseAccess;
    ConnectionString m_connectionString;
    std::shared_ptr<const Case> m_currentCase;

private:
    bool m_dictionaryAllowsExport;
    bool m_dictionaryUsesBinaryData;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
std::shared_ptr<T> CaseHoldingDoc::GetSettings()
{
    Settings& settings = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSettings();

    // a unique copy of settings will be created for case-only documents as multiple
    // cases belonging to a single data source may be open with different settings
    return settings.GetSettings<T>(m_connectionString, IsDocumentCaseOnly());
}
