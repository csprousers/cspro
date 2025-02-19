#pragma once

#include <zAppO/AppFileType.h>
#include <zUtilF/SystemIcon.h>


class AppFileTypeImageList : public SystemIcon::ImageList
{
public:
    void AddIcon(unsigned icon_resource_id, AppFileType app_file_type);

    int GetImageIndex(AppFileType app_file_type, const std::string& path);

private:
    std::map<AppFileType, int> m_imageIndexMap;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void AppFileTypeImageList::AddIcon(const unsigned icon_resource_id, const AppFileType app_file_type)
{
    m_imageIndexMap.try_emplace(app_file_type, GetImageCount());
    Add(AfxGetApp()->LoadIcon(icon_resource_id));
}


inline int AppFileTypeImageList::GetImageIndex(const AppFileType app_file_type, const std::string& path)
{
    if( app_file_type == AppFileType::Resource )
        return GetIconIndexFromPath(path);

    const auto& lookup = m_imageIndexMap.find(app_file_type);
    return ( lookup != m_imageIndexMap.cend() ) ? lookup->second :
                                                  ReturnProgrammingError(-1);
}
