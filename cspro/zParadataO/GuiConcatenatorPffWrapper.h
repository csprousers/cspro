#pragma once

#include <zParadataO/GuiConcatenator.h>
#include <zAppO/PFF.h>


namespace Paradata
{
    class GuiConcatenatorPffWrapper : public PffWrapper
    {
    public:
        GuiConcatenatorPffWrapper(const PFF& pff)
            :   m_pff(pff)
        {
        }

        CString GetListingFName() const override
        {
            return m_pff.GetListingFName();
        }

        std::vector<std::wstring> GetInputParadataFilenames() const override
        {
            std::vector<std::wstring> filenames;

            for( const CString& filename : m_pff.GetInputParadataFilenames() )
                filenames.emplace_back(filename);

            return filenames;
        }

        CString GetOutputParadataFilename() const override
        {
            return m_pff.GetOutputParadataFilename();
        }

        void ViewListing(bool errors_occurred) const override
        {
            if( m_pff.GetViewListing() == ALWAYS || ( m_pff.GetViewListing() == ONERROR && errors_occurred ) )
                m_pff.ViewListing();
        }

    private:
        const PFF& m_pff;
    };
}
