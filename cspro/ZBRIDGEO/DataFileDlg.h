#pragma once

#include <ZBRIDGEO/zBridgeO.h>
#include <ZBRIDGEO/DataFileFilterManager.h>

class WinRegistry;


// a dialog for selecting data files

class CLASS_DECL_ZBRIDGEO DataFileDlg : public CFileDialog, public IFileDialogEvents
{
public:
    enum class Type { OpenExisting, OpenOrCreate, CreateNew };

    DataFileDlg(Type type, bool add_only_readable_types, ConnectionString connection_string = ConnectionString());
    DataFileDlg(Type type, bool add_only_readable_types, const std::vector<ConnectionString>& connection_strings);
    virtual ~DataFileDlg();

    DataFileDlg& SetTitle(const CString& title);
    DataFileDlg& SetDictionaryFilename(const CString& dictionary_filename);
    DataFileDlg& SuggestMatchingDataRepositoryType(const ConnectionString& connection_string);
    DataFileDlg& SuggestMatchingDataRepositoryType(const std::vector<ConnectionString>& connection_strings);
    DataFileDlg& WarnIfDifferentDataRepositoryType();
    DataFileDlg& SetCreateNewDefaultDataRepositoryType(DataRepositoryType type);
    DataFileDlg& AllowMultipleSelections();

    const ConnectionString& GetConnectionString() const               { return m_selectedConnectionStrings.front(); }
    const std::vector<ConnectionString>& GetConnectionStrings() const { return m_selectedConnectionStrings; }

    IFACEMETHODIMP OnFileOk(IFileDialog*) { return S_OK; }
    IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; }
    IFACEMETHODIMP OnFolderChange(IFileDialog* pfd);
    IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; }
    IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; }
    IFACEMETHODIMP OnTypeChange(IFileDialog*) { return S_OK; }
    IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE* ) { return S_OK; }
    IFACEMETHODIMP QueryInterface(REFIID, void __RPC_FAR* __RPC_FAR*) { return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() { return S_OK; }
    ULONG STDMETHODCALLTYPE Release() { return S_OK; }

    INT_PTR DoModal() override;

protected:
    virtual BOOL OnFileNameOK() override;

private:
    bool AllowingMultipleSelection() const { return ( m_multipleSelectionBuffer != nullptr ); }

    IFileDialog* GetIFileDialog();
    static LRESULT CALLBACK DataFileDlgSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uidSubclass, DWORD_PTR dwData);

    void UpdateInitialDirectory();

    void UpdateFilters();

    bool ValidateConnectionStringText(ConnectionString& connection_string);

    WinRegistry* GetWinRegistry();
    CString GetDictionaryRegistryKeyName() const;

private:
    const Type m_type;
    const DataFileFilterManager m_dataFileFilterManager;

    const ConnectionString m_initialConnectionString;
    CString m_initialMultipleSelectionFilename;
    CString m_dictionaryFilename;
    ConnectionString m_suggestedMatchingDataRepositoryTypeConnectionString;
    bool m_warnIfDifferentDataRepositoryType;
    DataRepositoryType m_createNewDefaultDataRepositoryType;

    std::unique_ptr<TCHAR[]> m_multipleSelectionBuffer;

    std::vector<ConnectionString> m_selectedConnectionStrings;

    CString m_title;
    CString m_initialDirectory;
    std::unique_ptr<WinRegistry> m_winRegistry;

    bool m_mustSubclassDialog;
    DWORD m_fileDialogEventsHandlerCode;
    static DataFileDlg* m_currentDataFileDlg;
};
