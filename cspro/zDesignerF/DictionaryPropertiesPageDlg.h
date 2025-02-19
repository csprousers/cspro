#pragma once

#include <zAppO/DictionaryDescription.h>
#include <zUToolO/TreePropertiesPageValidator.h>

class ManageFilesDlg;


class DictionaryPropertiesPageDlg : public CDialog, public TreePropertiesPageValidator
{
public:
    static unsigned int GetDialogTemplateId();

    DictionaryPropertiesPageDlg(ManageFilesDlg& manage_files_dlg, DictionaryDescription dictionary_description,
                                std::shared_ptr<const CDataDict> dictionary, CWnd* pParent = nullptr);

    const DictionaryDescription& GetDictionaryDescription() const { return m_dictionaryDescription; }

    const CDataDict& GetDictionary() const { return *m_dictionary; }

    void OnValidatePage() override;

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

private:
    ManageFilesDlg& m_manageFilesDlg;
    DictionaryDescription m_dictionaryDescription;
    std::shared_ptr<const CDataDict> m_dictionary;

    std::wstring m_name;
    std::wstring m_label;

    RadioEnumHelper<DictionaryType> m_dictionaryTypeRadioEnumHelper;
    int m_dictionaryType;
    int m_includeInSimpleSynchronization;
    int m_includeValueSetImagesInCompiledApplication;
};
