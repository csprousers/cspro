#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/TextSource.h>


class CLASS_DECL_ZUTILO TextSourceExternal : public TextSource
{
public:
    TextSourceExternal(std::wstring filename);

    const std::wstring& GetText() const override;

    int64_t GetModifiedIteration() const override;

private:
    std::wstring m_text;
    mutable std::unique_ptr<std::tuple<int64_t, std::wstring>> m_iterationAndText;
};
