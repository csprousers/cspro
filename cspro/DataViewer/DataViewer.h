#pragma once

#include <engine/StandardSystemIncludes.h>
#include <engine/StrictCompilerErrors.h>


class DataViewerApp : public CWinApp
{
public:
    DataViewerApp();

protected:
    BOOL InitInstance() override;
};
