#include "StdAfx.h"
#include "DropFilesListCtrl.h"
#include <zToolsO/DirectoryLister.h>


BEGIN_MESSAGE_MAP(DropFilesListCtrl, CListCtrl)
    ON_WM_DROPFILES()
END_MESSAGE_MAP()


void DropFilesListCtrl::InitializeDropFiles(DirectoryHandling directory_handling, OnDropFilesCallback callback)
{
    ASSERT(( GetWindowLong(m_hWnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES ) != 0);

    m_directoryHandling = directory_handling;
    m_onDropFilesCallback = std::make_unique<OnDropFilesCallback>(std::move(callback));
}


void DropFilesListCtrl::SetNameFilter(const TCHAR* file_spec)
{
    m_directoryLister = std::make_shared<DirectoryLister>(true);

    m_directoryLister->SetNameFilter(file_spec);
}


void DropFilesListCtrl::SetNameFilter(std::shared_ptr<DirectoryLister> directory_lister)
{
    m_directoryLister = std::move(directory_lister);
}


void DropFilesListCtrl::OnDropFiles(HDROP hDropInfo)
{
    if( m_onDropFilesCallback == nullptr )
        return;

    bool filter_paths_using_name_filter = ( m_directoryLister != nullptr && m_directoryLister->UsingNameFilter() );
    std::vector<std::wstring> paths;

    UINT number_paths_dropped = DragQueryFile(hDropInfo, static_cast<UINT>(-1), nullptr, 0);

    for( UINT i = 0; i < number_paths_dropped; ++i )
    {
        TCHAR path[MAX_PATH];

        if( DragQueryFile(hDropInfo, i, path, MAX_PATH) != 0 )
        {
            if( m_directoryHandling != DirectoryHandling::AddToPaths && PortableFunctions::FileIsDirectory(path) )
            {
                if( m_directoryHandling == DirectoryHandling::RecurseInto )
                {
                    if( m_directoryLister == nullptr )
                        m_directoryLister = std::make_shared<DirectoryLister>(true);

                    m_directoryLister->AddPaths(paths, path);
                }                   
            }

            else
            {
                if( !filter_paths_using_name_filter || m_directoryLister->MatchesNameFilter(path) )
                    paths.emplace_back(path);
            }
        }
    }

    DragFinish(hDropInfo);

    if( !paths.empty() )
        (*m_onDropFilesCallback)(std::move(paths));
}
