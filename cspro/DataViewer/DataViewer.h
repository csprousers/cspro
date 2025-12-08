#pragma once

#include <engine/StandardSystemIncludes.h>
#include <StandardIncludes/strict_errors.h>


class DataViewerApp : public CWinApp
{
public:
    DataViewerApp();

protected:
    BOOL InitInstance() override;
};
