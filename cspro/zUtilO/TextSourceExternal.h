#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/TextSource.h>


class CLASS_DECL_ZUTILO TextSourceExternal : public TextSource
{
public:
    TextSourceExternal(std::string file_path);

    const std::string& GetText() const override;
    SharableString GetTextAsSharableString() const override;

    int64_t GetModifiedIteration() const override;

    bool RequiresSave() const override;

private:
    void SyncText() const;

private:
    mutable std::unique_ptr<std::tuple<int64_t, SharableString>> m_iterationAndText;
};
