#pragma once

#include <zIndexO/Indexer.h>


class ToolIndexer : public Indexer
{
protected:
    bool SupportsInteractiveMode() const override;
    void DisplayInteractiveModeMessage(NullTerminatedString message) const override;
    void ChooseDuplicate(std::vector<DuplicateInfo>& case_duplicates, size_t duplicate_index, size_t number_duplicates) const override;
    bool RethrowTerminatingException() const override;
};
