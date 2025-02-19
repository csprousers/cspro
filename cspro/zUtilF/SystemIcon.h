#pragma once

#include <zUtilF/zUtilF.h>


// --------------------------------------------------------------------------
// SystemIcon
// --------------------------------------------------------------------------

namespace SystemIcon
{
    class ImageList;

#ifdef WIN_DESKTOP
    CLASS_DECL_ZUTILF std::shared_ptr<const std::vector<std::byte>> GetPngForCSProLogo();

    CLASS_DECL_ZUTILF std::shared_ptr<const std::vector<std::byte>> GetPngForExtension(const std::string& extension);
    CLASS_DECL_ZUTILF std::shared_ptr<const std::vector<std::byte>> GetPngForFolder();
    CLASS_DECL_ZUTILF std::shared_ptr<const std::vector<std::byte>> GetPngForPath(const std::string& path);

#else
    std::shared_ptr<const std::vector<std::byte>> GetPngForExtension(std::string_view /*extension_sv*/) { return nullptr; }

#endif
};


#ifdef WIN_DESKTOP

// --------------------------------------------------------------------------
// SystemIcon::ImageList
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILF SystemIcon::ImageList : public CImageList
{
public:
    ImageList();
    ~ImageList();

    BOOL Create(int cx, int cy, UINT nFlags, int nInitial = 0, int nGrow = 2);

    // Adds an icon for the extension, if necessary, and returns the icon index.
    int GetIconIndexFromExtension(const std::string& extension);

    // Adds an icon for the extension, if necessary, and returns the icon index.
    // If the path points to a directory, a directory icon will be returned.
    int GetIconIndexFromPath(const std::string& path);

private:
    int m_iconSize;
    std::vector<std::tuple<std::string, HICON>> m_fileIcons;
    std::optional<int> m_folderIconIndex;
};

#endif
