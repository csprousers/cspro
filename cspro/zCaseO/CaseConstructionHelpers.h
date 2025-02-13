#pragma once

#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseItemReference.h>


namespace CaseConstructionHelpers
{
    inline std::shared_ptr<CaseItemReference> CreateCaseItemReference(const CaseAccess& case_access,
        const CString& level_key, const CString& field_name, const size_t occurrences[ItemIndex::NumberDimensions])
    {
        const CaseItem* case_item = case_access.LookupCaseItem(field_name);

        if( case_item != nullptr )
            return std::make_shared<CaseItemReference>(*case_item, level_key, occurrences);

        return nullptr;
    }

    inline std::shared_ptr<NamedReference> CreateNamedReference(const CaseAccess& case_access,
        const CString& level_key, const CString& field_name, const size_t occurrences[ItemIndex::NumberDimensions])
    {
        std::shared_ptr<NamedReference> named_reference = CreateCaseItemReference(case_access, level_key, field_name, occurrences);

        if( named_reference == nullptr )
            named_reference = std::make_shared<NamedReference>(field_name, level_key);

        return named_reference;
    }

    inline std::shared_ptr<NamedReference> CreateNamedReference(const CaseAccess& case_access,
        const CString& level_key, const CString& field_name)
    {
        static const size_t occurrences[ItemIndex::NumberDimensions] = { 0 };
        return CreateNamedReference(case_access, level_key, field_name, occurrences);
    }


    inline const CString& LookupCaseNote(const CString& dictionary_name, const std::vector<Note>& notes)
    {
        // search for the case note, which has the name of the dictionary
        for( const Note& note : notes )
        {
            if( dictionary_name.Compare(note.GetNamedReference().GetName()) == 0 )
                return note.GetContent();
        }

        return SO::EmptyCString;
    }
}
