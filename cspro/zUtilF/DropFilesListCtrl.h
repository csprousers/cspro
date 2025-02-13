#pragma once

#include <zUtilF/zUtilF.h>

class DirectoryLister;


class CLASS_DECL_ZUTILF DropFilesListCtrl : public CListCtrl
{
public:
    using OnDropFilesCallback = std::function<void(std::vector<std::wstring>)>;

    DropFilesListCtrl()
        :   m_directoryHandling(DirectoryHandling::RecurseInto)
    {
    }

    enum class DirectoryHandling { AddToPaths, RecurseInto, Ignore };

    void InitializeDropFiles(DirectoryHandling directory_handling, OnDropFilesCallback callback);

    template<typename T>
    void InitializeDropFiles(DirectoryHandling directory_handling, T name_filter, OnDropFilesCallback callback)
    {
        InitializeDropFiles(directory_handling, std::move(callback));
        SetNameFilter(std::move(name_filter));
    }

    void SetNameFilter(const TCHAR* file_spec);
    void SetNameFilter(std::shared_ptr<DirectoryLister> directory_lister);

protected:
    DECLARE_MESSAGE_MAP()

    void OnDropFiles(HDROP hDropInfo);

private:
    DirectoryHandling m_directoryHandling;
    std::shared_ptr<DirectoryLister> m_directoryLister;
    std::unique_ptr<OnDropFilesCallback> m_onDropFilesCallback;
};
