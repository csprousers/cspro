#pragma once

#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseItemReference.h>


namespace CaseConstructionHelpers
{
    std::unique_ptr<CaseItemReference> CreateCaseItemReference(const CaseAccess& case_access,
                                                               std::string level_key, const std::string& field_name,
                                                               const size_t occurrences[ItemIndex::NumberDimensions]);

    std::unique_ptr<NamedReference> CreateNamedReference(const CaseAccess& case_access,
                                                         std::string level_key, const std::string& field_name,
                                                         const size_t occurrences[ItemIndex::NumberDimensions]);

    std::unique_ptr<NamedReference> CreateNamedReference(const CaseAccess& case_access,
                                                         std::string level_key, const std::string& field_name);

    const std::string* LookupCaseNote(const std::string& dictionary_name, const std::vector<Note>* notes);
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::unique_ptr<CaseItemReference> CaseConstructionHelpers::CreateCaseItemReference(const CaseAccess& case_access,
                                                                                           std::string level_key, const std::string& field_name,
                                                                                           const size_t occurrences[ItemIndex::NumberDimensions])
{
    const CaseItem* const case_item = case_access.LookupCaseItem(field_name);

    if( case_item != nullptr )
        return std::make_unique<CaseItemReference>(*case_item, std::move(level_key), occurrences);

    return nullptr;
}


inline std::unique_ptr<NamedReference> CaseConstructionHelpers::CreateNamedReference(const CaseAccess& case_access,
                                                                                     std::string level_key, const std::string& field_name,
                                                                                     const size_t occurrences[ItemIndex::NumberDimensions])
{
    std::unique_ptr<NamedReference> named_reference = CreateCaseItemReference(case_access, level_key, field_name, occurrences);

    if( named_reference != nullptr )
        return named_reference;

    return std::make_unique<NamedReference>(field_name, std::move(level_key));
}


inline std::unique_ptr<NamedReference> CaseConstructionHelpers::CreateNamedReference(const CaseAccess& case_access,
                                                                                     std::string level_key, const std::string& field_name)
{
    static const size_t occurrences[ItemIndex::NumberDimensions] = { 0 };
    return CreateNamedReference(case_access, std::move(level_key), field_name, occurrences);
}


inline const std::string* CaseConstructionHelpers::LookupCaseNote(const std::string& dictionary_name, const std::vector<Note>* const notes)
{
    if( notes != nullptr && !notes->empty() )
    {
        // search for the case note, which has the name of the dictionary
        for( const Note& note : *notes )
        {
            if( dictionary_name == note.GetNamedReference().GetName() )
                return &note.GetContent();
        }
    }

    return nullptr;
}
