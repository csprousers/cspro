#pragma once

#include <zDesignerF/zDesignerF.h>

class ApplicationChildWnd;
class LogicSettings;
class TextSourceEditable;


// --------------------------------------------------------------------------
// DesignerDoc
//
// Documents that are edited in the Designer can derive from this class.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF DesignerDoc : public CDocument
{
    DECLARE_DYNAMIC(DesignerDoc)

public:
    virtual ~DesignerDoc() noexcept = default;

    // Returns the logic settings used by the document.
    virtual const LogicSettings& GetLogicSettings() = 0;

    // Returns a non-null pointer to the message text source, creating one
    // if necessary. TextSourceEditable does not throw exceptions on
    // construction when the file cannot be read, so it is assumed that this
    // method will not throw exceptions.
    virtual std::shared_ptr<TextSourceEditable> GetMessageTextSource() = 0;

    // Returns the ApplicationChildWnd associated with this document (if any).
    virtual ApplicationChildWnd* GetApplicationChildWnd() = 0;
};
