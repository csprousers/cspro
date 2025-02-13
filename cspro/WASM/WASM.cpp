#include <engine/StandardSystemIncludes.h>
#include <zPlatformO/PlatformInterface.h>


int main()
{
    try
    {
        PlatformInterface::GetInstance()->SetAssetsDirectory(_T("Assets"));
    }

    catch( const std::exception& e )
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
