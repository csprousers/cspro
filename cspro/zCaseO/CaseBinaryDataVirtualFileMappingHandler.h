#pragma once

#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/Case.h>
#include <zHtml/VirtualFileMapping.h>


class CaseBinaryDataVirtualFileMappingHandler : public KeyBasedVirtualFileMappingHandler
{
public:
    void SetCase(std::shared_ptr<const Case> data_case)
    {
        m_case = std::move(data_case);
        ASSERT(m_case != nullptr);
    }

    bool ServeContent(VirtualFileMappingResponse& response, const std::string& key) override
    {
        if( m_case == nullptr )
            return ReturnProgrammingError(false);

        const CaseItem* case_item;
        std::unique_ptr<CaseItemIndex> index;
        std::tie(case_item, index) = CaseItemIndex::FromSerializableText(*m_case, key);

        if( case_item != nullptr )
        {
            const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(*case_item);
            const BinaryData* binary_data = binary_case_item.GetBinaryData_noexcept(*index);

            if( binary_data != nullptr )
            {
                response.SetContent(binary_data->GetSharedContent(),
                                    ValueOrDefault(binary_data->GetMetadata().GetEvaluatedMimeType()));

                return true;
            }
        }

        return false;
    }

private:
    std::shared_ptr<const Case> m_case;
};
