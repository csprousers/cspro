#pragma once

#include <zDesignerF/zDesignerF.h>

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
    virtual ~DesignerDoc() = default;

    // Returns a non-null pointer to the message text source, creating one
    // if necessary. TextSourceEditable does not throw exceptions on
    // construction when the file cannot be read, so it is assumed that this
    // method will not throw exceptions.
    virtual std::shared_ptr<TextSourceEditable> GetMessageTextSource() = 0;
};
