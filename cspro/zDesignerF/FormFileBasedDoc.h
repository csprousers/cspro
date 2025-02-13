#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/ApplicationDoc.h>

class CDEFormFile;


class CLASS_DECL_ZDESIGNERF FormFileBasedDoc : public ApplicationDoc
{
    DECLARE_DYNAMIC(FormFileBasedDoc)

protected:
    FormFileBasedDoc();

public:
    const CDEFormFile& GetFormFile() const                       { return *m_formFile; }
    CDEFormFile& GetFormFile()                                   { return *m_formFile; }
    std::shared_ptr<const CDEFormFile> GetSharedFormFile() const { return m_formFile; }
    std::shared_ptr<CDEFormFile> GetSharedFormFile()             { return m_formFile; }

protected:    
    std::shared_ptr<CDEFormFile> m_formFile;
};
