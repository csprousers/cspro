#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zUToolO/TreePrps.h>
#include <zAppO/Application.h>
#include <afxmenubutton.h>

class AppFileTypeImageList;
class ApplicationLoader;
class ApplicationPropertiesPageDlg;
class OpenFileDlg;


class CLASS_DECL_ZDESIGNERF ManageFilesDlg : public CTreePropertiesDlg
{
public:
    // Unless the dialog returns IDOK, the application object may be in a bad state
    // because application objects are removed from it on validation and then added back when valid.
    ManageFilesDlg(Application& application, ApplicationLoader& application_loader,
                   std::unique_ptr<AppFileTypeImageList> app_file_type_image_list, CWnd* pParent = nullptr);

    ~ManageFilesDlg();

    void InitiallySelectPath(const std::string& path);

    const std::vector<std::tuple<std::string, AppFileType>>& GetFilesAdded() const   { return m_filesAdded; }
    const std::vector<std::tuple<std::string, AppFileType>>& GetFilesRemoved() const { return m_filesRemoved; }

    const std::set<std::string>& GetNonApplicationObjectsModified() const { return m_nonApplicationObjectsModified; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    void OnSize(UINT nType, int cx, int cy);

    BOOL PreTranslateMessage(MSG* pMsg) override;
    BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

    void OnHelp();

    void ResizeDlg(const CRect& new_page_rect) override;

    void OnModifyTreeItemBeforeInsert(CDialog* dlg, TVITEMW& item) override;

    bool OnPageChange(CDialog* old_page, CDialog* new_page) override;

    void OnAddFile();
    void OnRemoveFile();

    void OnOK() override;

private:
    // These methods are intended to be called by the various property pages:
    friend class ApplicationPropertiesPageDlg;
    friend class DictionaryPropertiesPageDlg;
    friend class FormFilePropertiesPageDlg;
    friend class ReportPropertiesPageDlg;
    friend class TableSpecPropertiesPageDlg;

    // Updates the page's name in the tree (if showing names).
    // The function assumes that the caller is the page currently selected by the tree.
    void UpdateNameInTree(const CDialog& dialog, std::wstring new_name);

    // Validates a name, ensuring that it is a valid CSPro name, as well as making sure that the name
    // is not duplicated against the objects that are part of the application, throwing an exception on error.
    // If path is not empty, validation stops after processing the object with the specified path.
    void ValidateName(const std::string& name, const std::string& path) const;

private:
    void SizeCurrentDialogToMatchPagePlaceholder();

    struct ManagePage;

    // If parent_page_or_type is set to AppFileType, the page will be added after existing pages of the same type.
    template<typename T>
    CDialog* AddManagePage(std::shared_ptr<T> dialog, std::variant<CDialog*, AppFileType> parent_page_or_type,
                           AppFileType app_file_type, std::string_view name_sv, const std::string& path,
                           bool can_remove_file);

    CDialog* GetManagePageInsertionPoint(AppFileType app_file_type) const;

    ManagePage* GetManagePage(const CDialog* dialog);
    ManagePage* GetManagePage(HTREEITEM hItem);

    void BuildTree();

    void BuildTreeApplication();

    void BuildTreeFormFile(CDialog* parent_page, AppFileType app_file_type, bool main_form_file);
    CDialog* BuildTreeFormFile(std::variant<CDialog*, AppFileType> parent_page_or_type, AppFileType app_file_type, bool main_form_file,
                               const std::string& form_file_path,
                               std::variant<std::shared_ptr<CDEFormFile>, std::shared_ptr<const CDEFormFile>> form_file,
                               std::shared_ptr<const CDataDict> dictionary);

    void BuildTreeTableSpec(CDialog* parent_page);

    CDialog* BuildTreeDictionary(std::variant<CDialog*, AppFileType> parent_page_or_type, DictionaryDescription dictionary_description,
                                 std::shared_ptr<const CDataDict> dictionary, bool can_remove_file);

    void BuildTreeCode(CDialog* parent_page, bool main_logic);
    CDialog* BuildTreeCode(std::variant<CDialog*, AppFileType> parent_page_or_type, const CodeFile& code_file);

    void BuildTreeMessages(CDialog* parent_page, bool main_message_file);
    CDialog* BuildTreeMessages(std::variant<CDialog*, AppFileType> parent_page_or_type, const AppMessageFile& app_message_file, bool main_message_file);

    CDialog* BuildTreeReport(std::variant<CDialog*, AppFileType> parent_page_or_type, const ReportFile& report_file);

    CDialog* BuildTreeResource(std::variant<CDialog*, AppFileType> parent_page_or_type, const AppResource& resource);

    std::unique_ptr<OpenFileDlg> CreateOpenFileDlg(const wchar_t* title, bool allow_create, bool allow_multi_select,
                                                   const char* default_extension, std::wstring filter);

    void OnAddFormFile();

    DictionaryDescription GetOrCreateDictionaryDescription(const std::string& dictionary_file_path, const std::string& parent_file_path,
                                                           DictionaryType dictionary_type_if_creating) const;

    bool UsesDefaultWorkingStoringDictionary() const;

    void CheckDictionaryIsUnused(const std::string& dictionary_file_path) const;

    CDialog* AddExternalDictionary(const std::string& dictionary_file_path, DictionaryType dictionary_type);
    void OnAddDictionaryExternal();
    void OnAddDictionaryWorking();
    void OnRemoveDictionary(const ManagePage& manage_page);

    void OnAddCode();

    void OnAddMessages();

    void OnAddReport();
    static bool CreateDefaultHtmlReport(const std::string& report_file_path);

    void OnAddResourceDirectory();
    void OnAddResourceFile();

    static void CheckNameIsUnused(const std::string& name, const ApplicationPropertiesPageDlg& application_properties_page_dlg);
    static void CheckNameIsUnused(const std::string& name, const FormFilePropertiesPageDlg& form_file_properties_page_dlg);
    static void CheckNameIsUnused(const std::string& name, const TableSpecPropertiesPageDlg& table_spec_properties_page_dlg);
    static void CheckNameIsUnused(const std::string& name, const CDataDict& dictionary);
    static void CheckNameIsUnused(const std::string& name, const ReportFile& report_file);

    std::string CreateUniqueName(const std::string& initial_name_candidate) const;

private:
    Application& m_application;
    ApplicationLoader& m_applicationLoader;

    CSize m_initialWindowSize;

    bool m_showNamesInTree;

    std::unique_ptr<AppFileTypeImageList> m_appFileTypeImageList;
    CMFCMenuButton m_addFileButton;
    CMenu m_addFileMenu;

    struct ManagePage
    {
        std::shared_ptr<CDialog> dialog;
        int dialog_context_id;
        const CDialog* parent_page;
        AppFileType app_file_type;
        std::string path;
        bool can_remove_file;
        std::wstring tree_name;
        std::wstring tree_path;
    };

    std::vector<ManagePage> m_managePages;
    std::vector<std::shared_ptr<CDialog>> m_removedDialogs;

    std::vector<std::string> m_buildTreeErrors;
    const CDialog* m_initialPageToSelect;

    std::vector<std::tuple<std::string, AppFileType>> m_filesAdded;
    std::vector<std::tuple<std::string, AppFileType>> m_filesRemoved;
    std::set<std::string> m_nonApplicationObjectsModified;
};
