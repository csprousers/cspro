#pragma once

#include <zUtilF/zUtilF.h>

class DirectoryLister;


class CLASS_DECL_ZUTILF DropFilesListCtrl : public CListCtrl
{
public:
    using OnDropFilesCallback = std::function<void(std::vector<std::string>)>;

    DropFilesListCtrl();

    enum class DirectoryHandling { AddToPaths, RecurseInto, Ignore };

    void InitializeDropFiles(DirectoryHandling directory_handling, OnDropFilesCallback callback);

    template<typename T>
    void InitializeDropFiles(DirectoryHandling directory_handling, T&& name_filter, OnDropFilesCallback callback);

    void SetNameFilter(std::string_view file_spec_sv);
    void SetNameFilter(std::shared_ptr<DirectoryLister> directory_lister);

protected:
    DECLARE_MESSAGE_MAP()

    void OnDropFiles(HDROP hDropInfo);

private:
    DirectoryHandling m_directoryHandling;
    std::shared_ptr<DirectoryLister> m_directoryLister;
    std::unique_ptr<OnDropFilesCallback> m_onDropFilesCallback;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline DropFilesListCtrl::DropFilesListCtrl()
    :   m_directoryHandling(DirectoryHandling::RecurseInto)
{
}


template<typename T>
void DropFilesListCtrl::InitializeDropFiles(const DirectoryHandling directory_handling, T&& name_filter, OnDropFilesCallback callback)
{
    InitializeDropFiles(directory_handling, std::move(callback));
    SetNameFilter(std::forward<T>(name_filter));
}
