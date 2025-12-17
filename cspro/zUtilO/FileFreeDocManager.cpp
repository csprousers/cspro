#include "StdAfx.h"
#include "FileFreeDocManager.h"
#include "WindowsWS.h"
#include <zToolsO/CaseInsensitiveComparer.h>
#include <../src/mfc/afximpl.h>


struct FileFreeDocManager::TemplateData
{
    CDocTemplate* doc_template;
    std::string dummy_filename_prefix;
    std::optional<std::set<std::wstring, cs::case_insensitive_less>> extensions;
};


FileFreeDocManager::FileFreeDocManager()
{
}


FileFreeDocManager::~FileFreeDocManager()
{
}


void FileFreeDocManager::AddDocTemplate(const UINT nIDResource, CDocTemplate* const doc_template)
{
    ASSERT(m_docTemplates.find(nIDResource) == m_docTemplates.cend());
    ASSERT(doc_template != nullptr);

    m_docTemplates[nIDResource].doc_template = doc_template;

    __super::AddDocTemplate(doc_template);
}


std::wstring FileFreeDocManager::EvaluateFilename(LPCTSTR lpszFileName)
{
    // this code is adapted from CDocManager::OpenDocumentFile
    if( lpszFileName == nullptr )
        AfxThrowInvalidArgException();

    wchar_t szPath[_MAX_PATH];
    ASSERT(AtlStrLen(lpszFileName) < _countof(szPath));
    wchar_t szTemp[_MAX_PATH];
    if (lpszFileName[0] == '\"')
        ++lpszFileName;
    Checked::tcsncpy_s(szTemp, _countof(szTemp), lpszFileName, _TRUNCATE);
    LPTSTR lpszLast = _tcsrchr(szTemp, '\"');
    if (lpszLast != NULL)
        *lpszLast = 0;

    if( AfxFullPath(szPath, szTemp) == FALSE )
    {
        ASSERT(FALSE);
        return NULL; // We won't open the file. MFC requires paths with
                     // length < _MAX_PATH
    }

    if( AfxResolveShortcut(AfxGetMainWnd(), szPath, szTemp, _MAX_PATH) )
        return szTemp;

    return szPath;
}


void FileFreeDocManager::ActivateDocument(CDocument* const pDoc)
{
    // this code is adapted from CDocManager::OpenDocumentFile
    ASSERT(pDoc != nullptr);

    POSITION posOpenDoc = pDoc->GetFirstViewPosition();
    if (posOpenDoc != NULL)
    {
        CView* pView = pDoc->GetNextView(posOpenDoc); // get first one
        ASSERT_VALID(pView);
        CFrameWnd* pFrame = pView->GetParentFrame();

        if (pFrame == NULL)
            TRACE(traceAppMsg, 0, "Error: Can not find a frame for document to activate.\n");
        else
        {
            pFrame->ActivateFrame();

            if (pFrame->GetParent() != NULL)
            {
                CFrameWnd* pAppFrame;
                if (pFrame != (pAppFrame = (CFrameWnd*)AfxGetApp()->m_pMainWnd))
                {
                    ASSERT_KINDOF(CFrameWnd, pAppFrame);
                    pAppFrame->ActivateFrame();
                }
            }
        }
    }
    else
        TRACE(traceAppMsg, 0, "Error: Can not find a view for document to activate.\n");
}


CDocument* FileFreeDocManager::FindAndActivateOpenDocumentByPath(const CDocTemplate* const doc_template, const wchar_t* path)
{
    ASSERT(doc_template != nullptr && path != nullptr);

    POSITION doc_pos = doc_template->GetFirstDocPosition();

    while( doc_pos != nullptr )
    {
        CDocument* const open_doc = doc_template->GetNextDoc(doc_pos);

        if( AfxComparePath(open_doc->GetPathName(), path) )
        {
            ActivateDocument(open_doc);
            return open_doc;
        }
    }

    return nullptr;
}


CDocument* FileFreeDocManager::FindAndActivateOpenDocumentByPath(const wchar_t* const path) const
{
    POSITION template_pos = m_templateList.GetHeadPosition();

    while( template_pos != nullptr )
    {
        const CDocTemplate* const doc_template = static_cast<const CDocTemplate*>(m_templateList.GetNext(template_pos));
        ASSERT_KINDOF(CDocTemplate, doc_template);

        CDocument* const open_doc = FindAndActivateOpenDocumentByPath(doc_template, path);

        if( open_doc != nullptr )
            return open_doc;
    }

    return nullptr;
}


CDocument* FileFreeDocManager::FindAndActivateOpenDocumentOfType(const CDocTemplate* const doc_template)
{
    POSITION doc_pos = doc_template->GetFirstDocPosition();

    if( doc_pos != nullptr )
    {
        CDocument* const open_doc = doc_template->GetNextDoc(doc_pos);
        ActivateDocument(open_doc);
        return open_doc;
    }

    return nullptr;
}


std::wstring FileFreeDocManager::CreateDummyFilePath(TemplateData& template_data) const
{
    if( template_data.dummy_filename_prefix.empty() )
    {
        // use the document string's CDocTemplate::windowTitle value for the dummy
        // filenames created for this document, defaulting to the resource ID if not set
        CString window_title;

        if( template_data.doc_template->GetDocString(window_title, CDocTemplate::DocStringIndex::windowTitle) &&
            !window_title.IsEmpty() )
        {
            template_data.dummy_filename_prefix = TC::ToUtf8(window_title);
            template_data.dummy_filename_prefix.push_back('-');
            SO::Replace(template_data.dummy_filename_prefix, ' ', '-');
        }

        else
        {
            template_data.dummy_filename_prefix = FormatText("Doc%p-", static_cast<const void*>(template_data.doc_template));
        }

        Path::MakeValidFilename(template_data.dummy_filename_prefix);

        ASSERT(std::count_if(m_docTemplates.cbegin(), m_docTemplates.cend(),
                             [&](const auto& td) { return ( td.second.dummy_filename_prefix == template_data.dummy_filename_prefix ); }) == 1);
    }

    const std::string file_path = PortableFunctions::GetUniqueFilePathInDirectory(
        GetTempDirectory(),
        std::string_view(), // no extension
        template_data.dummy_filename_prefix.c_str()
    );

    ASSERT(!PortableFunctions::FileExists(file_path));

    return TC::ToWide(file_path);
}


CDocTemplate* FileFreeDocManager::FindDocTemplateByExtension(const std::wstring& extension)
{
    if( extension.empty() )
        return nullptr;

    ASSERT(extension.front() != '.');

    for( auto& [resource_id, template_data] : m_docTemplates )
    {
        // calculate the extensions that this document template supports
        if( !template_data.extensions.has_value() )
        {
            template_data.extensions.emplace();

            CString filter_ext;
            template_data.doc_template->GetDocString(filter_ext, CDocTemplate::DocStringIndex::filterExt);

            SO::ForeachSection(std::wstring_view(filter_ext), ';',
                [&](std::wstring_view extension_sv)
                {
                    if( extension_sv.size() < 2 || extension_sv.front() != '.' )
                    {
                        ASSERT(false);
                        return;
                    }

                    extension_sv.remove_prefix(1);

                    ASSERT(std::find_if(m_docTemplates.cbegin(), m_docTemplates.cend(),
                        [&](const auto& td) { return ( td.second.extensions.has_value() &&
                                                       td.second.extensions->count(extension_sv) != 0 ); }) == m_docTemplates.cend());

                    template_data.extensions->emplace(extension_sv);
                });
        }

        if( template_data.extensions->find(extension) != template_data.extensions->cend() )
            return template_data.doc_template;
    }

    return nullptr;
}


CDocument* FileFreeDocManager::OpenDocumentFile(LPCTSTR lpszFileName, const BOOL bAddToMRU)
{
    // this code is adapted from CDocManager::OpenDocumentFile + CDocTemplate::MatchDocType
    const std::wstring file_path = EvaluateFilename(lpszFileName);

    // check if the document is already open
    CDocument* const open_doc = FindAndActivateOpenDocumentByPath(file_path.c_str());

    if( open_doc != nullptr )
        return open_doc;

    // if not, see if it matches a file-based document template by comparing extensions
    const std::wstring extension = PortableFunctions::PathGetFileExtension(file_path);
    CDocTemplate* const doc_template = FindDocTemplateByExtension(extension);

    if( doc_template != nullptr )
        return doc_template->OpenDocumentFile(file_path.c_str(), bAddToMRU, TRUE);

    // otherwise fail and display an error message
    std::wstring error_message = WindowsWS::LoadString(AFX_IDS_APP_TITLE);

    if( error_message.empty() )
        error_message = L"This application";

    if( extension.empty() )
    {
        error_message.append(L" cannot open this document.");
    }

    else
    {
        error_message.append(L" cannot open a document with the extension: .")
                     .append(extension);
    }

    AfxMessageBox(error_message);

    return nullptr;
}


CDocument* FileFreeDocManager::Open(const UINT nIDResource, const bool always_create_new_document)
{
    const auto& lookup = m_docTemplates.find(nIDResource);

    if( lookup == m_docTemplates.cend() )
        return ReturnProgrammingError(nullptr);

    TemplateData& template_data = lookup->second;

    // if not creating a new document, activate an open document of this type when possible
    if( !always_create_new_document )
    {
        CDocument* const open_doc = FindAndActivateOpenDocumentOfType(template_data.doc_template);

        if( open_doc != nullptr )
            return open_doc;
    }

    const std::wstring file_path = CreateDummyFilePath(template_data);

    return template_data.doc_template->OpenDocumentFile(file_path.c_str(), FALSE, TRUE);
}
