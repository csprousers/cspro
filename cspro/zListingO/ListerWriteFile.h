#pragma once

#include <zListingO/WriteFile.h>


namespace Listing
{
    template<typename T>
    class ListerWriteFile : public WriteFile
    {
    public:
        ListerWriteFile(T lister)
            :   m_lister(std::move(lister))
        {
            ASSERT(m_lister != nullptr);
        }

        void WriteLine(std::wstring text) override
        {
            m_lister->WriteLineForWriteFile(std::move(text));
        }

    private:
        T m_lister;
    };
}
