#pragma once

#include <zBridgeO/zBridgeO.h>
#include <zBridgeO/DataFileFilterManager.h>

class WinRegistry;


// a dialog for selecting data files

class CLASS_DECL_ZBRIDGEO DataFileDlg : public CFileDialog, public IFileDialogEvents
{
public:
    enum class Type { OpenExisting, OpenOrCreate, CreateNew };

    DataFileDlg(Type type, bool add_only_readable_types, ConnectionString connection_string = ConnectionString(), CWnd* pParentWnd = nullptr);
    DataFileDlg(Type type, bool add_only_readable_types, const std::vector<ConnectionString>& connection_strings, CWnd* pParentWnd = nullptr);
    virtual ~DataFileDlg();

    DataFileDlg& SetTitle(std::wstring title);
    DataFileDlg& SetTitle(std::string_view title_sv);
    DataFileDlg& SetDictionaryFilePath(std::string dictionary_file_path);
    DataFileDlg& SuggestMatchingDataRepositoryType(const ConnectionString& connection_string);
    DataFileDlg& SuggestMatchingDataRepositoryType(const std::vector<ConnectionString>& connection_strings);
    DataFileDlg& WarnIfDifferentDataRepositoryType();
    DataFileDlg& SetCreateNewDefaultDataRepositoryType(DataRepositoryType type);
    DataFileDlg& AllowMultipleSelections();

    const ConnectionString& GetConnectionString() const               { return m_selectedConnectionStrings.front(); }
    const std::vector<ConnectionString>& GetConnectionStrings() const { return m_selectedConnectionStrings; }

    IFACEMETHODIMP OnFileOk(IFileDialog*) override { return S_OK; }
    IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) override { return S_OK; }
    IFACEMETHODIMP OnFolderChange(IFileDialog* pfd) override ;
    IFACEMETHODIMP OnSelectionChange(IFileDialog*) override { return S_OK; }
    IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) override { return S_OK; }
    IFACEMETHODIMP OnTypeChange(IFileDialog*) override { return S_OK; }
    IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) override { return S_OK; }
    IFACEMETHODIMP QueryInterface(REFIID, void __RPC_FAR* __RPC_FAR*) override { return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() override { return S_OK; }
    ULONG STDMETHODCALLTYPE Release() override { return S_OK; }

    INT_PTR DoModal() override;

    static std::optional<ConnectionString> ShowDialogFromWinForms(CWnd* pParentWnd, Type type, bool add_only_readable_types, ConnectionString connection_string);

protected:
    BOOL OnFileNameOK() override;

private:
    bool AllowingMultipleSelection() const { return ( m_multipleSelectionBuffer != nullptr ); }

    static const DataFileFilterManager& GetDataFileFilterManager(bool add_only_readable_types);

    IFileDialog* GetIFileDialog();

    static bool IsValidDataFilename(const std::string& filename, bool allow_wildcards);

    static LRESULT CALLBACK DataFileDlgSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uidSubclass, DWORD_PTR dwData);

    void UpdateInitialDirectory();

    void UpdateFilters();

    bool ValidateConnectionStringText(ConnectionString& connection_string);

    WinRegistry* GetWinRegistry();
    std::string GetDictionaryRegistryKeyName() const;

private:
    const Type m_type;
    const DataFileFilterManager m_dataFileFilterManager;

    const ConnectionString m_initialConnectionString;
    CString m_initialMultipleSelectionFilename;
    std::string m_dictionaryFilePath;
    ConnectionString m_suggestedMatchingDataRepositoryTypeConnectionString;
    bool m_warnIfDifferentDataRepositoryType;
    DataRepositoryType m_createNewDefaultDataRepositoryType;

    std::unique_ptr<wchar_t[]> m_multipleSelectionBuffer;

    std::vector<ConnectionString> m_selectedConnectionStrings;

    std::wstring m_title;
    std::wstring m_initialDirectory;
    std::unique_ptr<WinRegistry> m_winRegistry;

    bool m_mustSubclassDialog;
    DWORD m_fileDialogEventsHandlerCode;
    static DataFileDlg* m_currentDataFileDlg;
};
