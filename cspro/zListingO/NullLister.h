#pragma once

#include <zListingO/Lister.h>


namespace Listing
{
    class NullLister : public Lister
    {
    public:
        NullLister(std::shared_ptr<ProcessSummary> process_summary)
            :   Lister(std::move(process_summary))
        {
        }

    protected:
        void WriteMessages(const Messages& /*messages*/) override
        {
        }
    };
}
