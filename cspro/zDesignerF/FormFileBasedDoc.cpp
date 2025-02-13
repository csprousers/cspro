#include "StdAfx.h"
#include "FormFileBasedDoc.h"


IMPLEMENT_DYNAMIC(FormFileBasedDoc, ApplicationDoc)


FormFileBasedDoc::FormFileBasedDoc()
    :   m_formFile(std::make_shared<CDEFormFile>())
{
}
