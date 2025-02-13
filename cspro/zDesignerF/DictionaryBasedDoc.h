#pragma once

#include <zDesignerF/zDesignerF.h>

class CDataDict;


class CLASS_DECL_ZDESIGNERF DictionaryBasedDoc : public CDocument
{
    DECLARE_DYNAMIC(DictionaryBasedDoc)

public:
    const CTreeCtrl* GetTreeCtrl() const { return const_cast<DictionaryBasedDoc*>(this)->GetTreeCtrl(); }

    void SetModifiedFlag(BOOL modified = TRUE) override;

    // methods for subclasses to override
    virtual CTreeCtrl* GetTreeCtrl() = 0;
    virtual std::shared_ptr<const CDataDict> GetSharedDictionary() const = 0;
};
