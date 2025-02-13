#pragma once

#include <zUtilF/zUtilF.h>


namespace SystemIcon
{
#ifdef WIN_DESKTOP
    CLASS_DECL_ZUTILF std::shared_ptr<const std::vector<std::byte>> GetPngForCSProLogo();

    CLASS_DECL_ZUTILF std::shared_ptr<const std::vector<std::byte>> GetPngForExtension(wstring_view extension);
    CLASS_DECL_ZUTILF std::shared_ptr<const std::vector<std::byte>> GetPngForFolder();
    CLASS_DECL_ZUTILF std::shared_ptr<const std::vector<std::byte>> GetPngForPath(NullTerminatedString path);

    class CLASS_DECL_ZUTILF ImageList : public CImageList
    {
    public:
        ImageList();
        ~ImageList();

        BOOL Create(int cx, int cy, UINT nFlags);

        // adds an icon for the extension, if necessary, and returns the icon index
        int GetIconIndexFromExtension(wstring_view extension);

        // adds an icon for the extension, if necessary, and returns the icon index;
        // if the path points to a directory, a directory icon will be returned
        int GetIconIndexFromPath(NullTerminatedString path);

    private:
        int m_iconSize;
        std::vector<std::tuple<size_t, HICON>> m_fileIcons;
        std::optional<int> m_folderIconIndex;
    };

#else
    std::shared_ptr<const std::vector<std::byte>> GetPngForExtension(wstring_view /*extension*/) { return nullptr; }

#endif
};
