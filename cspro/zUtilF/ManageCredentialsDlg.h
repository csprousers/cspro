#pragma once

#include <zUtilF/zUtilF.h>
#include <zUtilO/ResizableDlg.h>

class CMFCMenuButton;


class CLASS_DECL_ZUTILF ManageCredentialsDlg : public ResizableDlgEx
{
public:
    ManageCredentialsDlg(CWnd* pParent = nullptr);
    ~ManageCredentialsDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnCredentialSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult);

    void OnClear();
    void OnClearAll();

private:
    struct Credential;
    enum class CredentialType { Sync, Data, Location };

    static const wchar_t* ToString(CredentialType credential_type, bool for_details);

    void GetCredentials();
    void PopulateCredentialsTree(const Credential* credential_to_select);

    static void SetUpCredentialSync(Credential& credential);
    static void SetUpCredentialSync_OAuth2Token(Credential& credential, const JsonNode& json_node);

    static void SetUpCredentialData(Credential& credential);

    static void SetUpCredentialLocation(Credential& credential);

    std::variant<std::monostate, CredentialType, const Credential*> GetCurrentSelection() const;

    void SetDetailsText();

    static bool ConfirmClear(size_t number_credentials);

    void ClearCredentialsAll();
    void ClearCredentials(CredentialType credential_type);
    void ClearCredential(const Credential* credential);

private:
    CTreeCtrl m_credentialsTreeCtrl;
    bool m_populatingCredentialsTreeCtrl;
    CStatic m_detailsText;
    std::unique_ptr<CMFCMenuButton> m_clearAllButton;
    CMenu m_clearAllMenu;
    std::map<CredentialType, std::vector<std::unique_ptr<Credential>>> m_credentials;
};
