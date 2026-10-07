#include <engine/StandardSystemIncludes.h>
#include <StandardIncludes/strict_errors.h>
#include <zToolsO/Utf8.h>
#include <zBuildTools/LicenseGenerator.h>


int wmain()
{
    std::string message;
    unsigned int message_flags = MB_OK;

    try
    {
        LicenseGenerator generator;
        const size_t number_licenses = generator.Generate();

        message = FormatText("Successfully created a combined license for %zu projects.", number_licenses);
    }

    catch( const CSProException& exception )
    {
        message = exception.what();
        message_flags |= MB_ICONEXCLAMATION;
    }

    if( !message.empty() )
        MessageBoxW(nullptr, TC::ToWide(message).c_str(), L"Generate Combined License", message_flags);
}
