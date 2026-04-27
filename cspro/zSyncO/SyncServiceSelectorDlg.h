#pragma once

#include <zSyncO/zSyncO.h>
#include <zUtilO/SyncConnectionString.h>
#include <zUtilF/DialogValidators.h>


// A child dialog, for embedding in other dialogs, that allows for selecting a sync service.

class SYNC_API SyncServiceSelectorDlg : public CDialog
{
public:
    SyncServiceSelectorDlg(cs::cref_optional<SyncConnectionString> sync_connection_string, CWnd* pParent = nullptr);

    // Creates the child dialog, replacing the placeholder control given by ID.
    BOOL Create(CDialog* parent_dlg, UINT nID);

    // Validates and returns the sync connection string, throwing exceptions on error.
    // If save_url_to_registry is true, the sync connection string's safe representation
    // will saved to the registry for potential restoration when this class is used next.
    SyncConnectionString ValidateSyncConnectionString(bool save_url_to_registry = true) const;

    // Returns the entered sync connection string.
    SyncConnectionString GetSyncConnectionString(bool save_url_to_registry = true) const;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnSyncServiceChange(UINT nID);
    void OnDirectorySelect();

    // Called when the sync service type changes.
    LRESULT OnUpdateDialogUI(WPARAM wParam, LPARAM lParam);

private:
    template<typename T>
    static auto ConvertSyncServiceType(const T& value);

    // Sets the dialog UI based on the sync connection string. This does not call UpdateData.
    void ToForm(const SyncConnectionString& sync_connection_string, std::optional<int> sync_service_type_override);

    // Returns a sync connection string based on the dialog UI. This does not call UpdateData.
    SyncConnectionString FromForm(std::optional<int> sync_service_type_override) const;

    static void SaveUrlToRegistry(const SyncConnectionString& sync_connection_string);

private:
    int m_syncServiceType;
    std::string m_text; // URL, Dropbox email, or LocalFiles directory path
    int m_dropboxLocalType;
    std::map<int, SyncConnectionString> m_syncConnectionStringsPerServiceType;
};
