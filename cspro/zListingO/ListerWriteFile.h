#pragma once

#include <zListingO/Lister.h>
#include <zListingO/WriteFile.h>


class Listing::ListerWriteFile : public WriteFile
{
public:
    ListerWriteFile(cs::non_null_shared_or_raw_ptr<Lister> lister)
        :   m_lister(std::move(lister))
    {
    }

    void WriteLine(SharableString text) override
    {
        m_lister->WriteLineForWriteFile(std::move(text));
    }

private:
    cs::non_null_shared_or_raw_ptr<Lister> m_lister;
};
