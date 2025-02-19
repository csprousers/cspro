#include "StdAfx.h"
#include "DictionaryPropertiesPageDlg.h"
#include "ManageFilesDlg.h"


unsigned int DictionaryPropertiesPageDlg::GetDialogTemplateId()
{
    return IDD_PAGE_PROPERTIES_DICTIONARY;
}


DictionaryPropertiesPageDlg::DictionaryPropertiesPageDlg(ManageFilesDlg& manage_files_dlg, DictionaryDescription dictionary_description,
                                                         std::shared_ptr<const CDataDict> dictionary, CWnd* const pParent/* = nullptr*/)
    :   CDialog(GetDialogTemplateId(), pParent),
        m_manageFilesDlg(manage_files_dlg),
        m_dictionaryDescription(std::move(dictionary_description)),
        m_dictionary(std::move(dictionary)),
        m_name(UTF8_TODO::GetWide(m_dictionary->GetName())),
        m_label(CS2WS(m_dictionary->GetLabel())),
        m_dictionaryTypeRadioEnumHelper({ DictionaryType::Input,
                                          DictionaryType::External,
                                          DictionaryType::Working,
                                          DictionaryType::Output }),
        m_dictionaryType(m_dictionaryTypeRadioEnumHelper.ToForm(m_dictionaryDescription.GetDictionaryType())),
        m_includeInSimpleSynchronization(m_dictionaryDescription.GetIncludeInSimpleSynchronization()),
        m_includeValueSetImagesInCompiledApplication(m_dictionaryDescription.GetIncludeValueSetImagesInCompiledApplication())
{
}


void DictionaryPropertiesPageDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_NAME, m_name);
    DDX_Text(pDX, IDC_LABEL, m_label);
    DDX_Radio(pDX, IDC_MAIN, m_dictionaryType);
    DDX_Check(pDX, IDC_INCLUDE_IN_SIMPLE_SYNC, m_includeInSimpleSynchronization);
    DDX_Check(pDX, IDC_INCLUDE_IN_COMPILED_APPLICATION, m_includeValueSetImagesInCompiledApplication);
}


BOOL DictionaryPropertiesPageDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    auto enable_dictionary_types = [&](const BOOL input, const BOOL external, const BOOL working, const BOOL special_output)
    {
        GetDlgItem(IDC_MAIN)->EnableWindow(input);
        GetDlgItem(IDC_EXTERNAL)->EnableWindow(external);
        GetDlgItem(IDC_WORKING)->EnableWindow(working);
        GetDlgItem(IDC_SPECIAL_OUTPUT)->EnableWindow(special_output);
    };

    // the dictionary type cannot be modified for the application's input dictionary
    if( m_dictionaryDescription.GetDictionaryType() == DictionaryType::Input )
    {
        enable_dictionary_types(TRUE, FALSE, FALSE, FALSE);
    }

    // the dictionary type cannot be modified for dictionaries that belong to external forms
    else if( m_dictionaryDescription.GetDictionaryType() == DictionaryType::External &&
             !m_dictionaryDescription.GetParentFilePath().empty() )
    {
        enable_dictionary_types(FALSE, TRUE, FALSE, FALSE);
    }

    // otherwise all non-input dictionary types are valid, except that special output dictionaries are only valid in batch
    else
    {
        const BOOL is_batch = ( m_manageFilesDlg.m_application.GetApplicationAppFileType() == AppFileType::ApplicationBatch );
        enable_dictionary_types(FALSE, TRUE, TRUE, is_batch);
    }

    return result;
}


void DictionaryPropertiesPageDlg::OnValidatePage()
{
    UpdateData(TRUE);

    // validate the name (even though it isn't modifiable)
    m_manageFilesDlg.ValidateName(TC::ToUtf8(m_name), m_dictionaryDescription.GetDictionaryFilePath());

    // only input and external dictionaries can be part of a simple synchronization
    const DictionaryType dictionary_type = m_dictionaryTypeRadioEnumHelper.FromForm(m_dictionaryType);

    if( m_includeInSimpleSynchronization && ( dictionary_type != DictionaryType::Input &&
                                              dictionary_type != DictionaryType::External ) )
    {
        throw CSProException("Simple Synchronization is only valid for an application's main or external dictionaries.");
    }

    m_dictionaryDescription.SetDictionaryType(dictionary_type);
    m_dictionaryDescription.SetIncludeInSimpleSynchronization(m_includeInSimpleSynchronization);
    m_dictionaryDescription.SetIncludeValueSetImagesInCompiledApplication(m_includeValueSetImagesInCompiledApplication);
}
