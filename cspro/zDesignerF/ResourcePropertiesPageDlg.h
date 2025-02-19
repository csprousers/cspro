#pragma once

#include <zAppO/AppResource.h>
#include <zUToolO/TreePropertiesPageValidator.h>


class ResourcePropertiesPageDlg : public CDialog, public TreePropertiesPageValidator
{
public:
    static unsigned int GetDialogTemplateId();

    ResourcePropertiesPageDlg(AppResource resource, CWnd* pParent = nullptr);

    const AppResource& GetResource() const { return m_resource; }

    void OnValidatePage() override;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

private:
    bool IsDirectory() const { return ( m_type == 0 ); }

    void OnCalculateResourceFiles();

private:
    AppResource m_resource;

    int m_type;
    int m_includeInCompiledApplication;
    int m_recursive;
    std::string m_filenameFilter;
};
