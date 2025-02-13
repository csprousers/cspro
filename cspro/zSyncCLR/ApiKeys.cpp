#include "Stdafx.h"
#include "ApiKeys.h"
#include <zToolsO/ApiKeys.h>


System::String^ CSPro::Sync::DropboxKeys::client_id::get()
{
    return gcnew System::String(::DropboxKeys::client_id);
}


System::String^ CSPro::Sync::DropboxKeys::client_secret::get()
{
    return gcnew System::String(::DropboxKeys::client_secret);
}
