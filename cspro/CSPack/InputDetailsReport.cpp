#include "StdAfx.h"
#include "PackDlg.h"
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/MimeType.h>
#include <zHtml/HtmlViewDlg.h>
#include <zHtml/HtmlWriter.h>
#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zHtml/VirtualFileMapping.h>


namespace
{
    constexpr const TCHAR* ReportHeader = LR"(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Pack Application Input Details</title>
    <style>
        body {
            margin: 10px;
            padding: 0px;
            background-color: white;
            color: black;
            font-family: Arial, sans-serif;
            font-size: 10pt;
        }

        h1 {
            margin-bottom: 20px;
            color: green;
            font-size: 16pt;
        }

        table {
            width:100%;
        }

        tr.mainInput td {
            padding-top: 20px;
            font-weight: bold;
        }

        tr.dependentInput td {
            padding-top: 5px;
        }

        .thumbnailSmall {
            width: 24px;
        }

        .thumbnailSmall div {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 16px;
        }

        .thumbnailSmall img {
            max-width: 16px;
            max-height: 16px;
        }

        .thumbnailLarge {
            width: 128px;
        }

        .thumbnailLarge div {
            display: flex;
            justify-content: center;
            align-items: center;
        }

        .thumbnailLarge img {
            max-width: 128px;
        }

        .fileSize {
            color: #aaaaaa;
        }

        .pathShort {
            display: block;
        }

        .pathFull {
            display: none;
        }
    </style>
</head>
<body>
    <h1>Pack Application Input Details</h1>

    <p><b>The following inputs are included, along with any dependent files:</b></p>

    <div><input type="checkbox" id="showFullPaths"><label for="showFullPaths">Show full paths of dependent files</label></div>
    <div><input type="checkbox" id="showImages"><label for="showImages">Show image thumbnails and large icons</label></div>

    <table>
)";

    constexpr const TCHAR* ReportFooter = LR"(
    </table>

    <script>
        document.getElementById("showFullPaths").addEventListener("change", (event) => {
            function toggle(className, display) {
                Array.from(document.getElementsByClassName(className)).forEach(element => {
                    element.style.display = display ? "block" : "none";
                });
            }

            toggle("pathShort", !event.currentTarget.checked);
            toggle("pathFull", event.currentTarget.checked);
        });

        document.getElementById("showImages").addEventListener("change", (event) => {
            const classNames = [ "thumbnailSmall", "thumbnailLarge" ];
            const showLarge = event.currentTarget.checked;
            const elements = document.getElementsByClassName(classNames[showLarge ? 0 : 1]);

            while( elements && elements.length > 0 ) {
                elements[0].className = classNames[showLarge ? 1 : 0];
            }

            Array.from(document.getElementsByTagName("img")).forEach(element => {
                if( element.dataset.image ) {
                    [element.src, element.dataset.image] = [element.dataset.image, element.src];

                    element.onclick = function() {
                        if( showLarge ) {
                            window.open(element.src);
                        }
                    };
                }
            });
        });
    </script>

</body>
</html>
)";


    class IconPngProvider : public KeyBasedVirtualFileMappingHandler
    {
    public:
        bool ServeContent(void* response_object, const std::wstring& key) override
        {
            std::shared_ptr<const std::vector<std::byte>> png_data = SystemIcon::GetPngForPath(key);

            if( png_data != nullptr )
            {
                LocalFileServerSetResponse(response_object, *png_data, MimeType::Type::ImagePng);
                return true;
            }

            return false;
        }
    };
}


void PackDlg::DisplayInputDetailsReport()
{
    ASSERT(m_packSpec->GetNumEntries() > 0);

    static SharedHtmlLocalFileServer file_server;
    static std::unique_ptr<IconPngProvider> icon_png_provider;

    if( icon_png_provider == nullptr )
    {
        icon_png_provider = std::make_unique<IconPngProvider>();
        file_server.CreateVirtualDirectory(*icon_png_provider);
    }

    HtmlStringWriter html_writer;

    html_writer << ReportHeader;

    for( const PackEntry& pack_entry : m_packSpec->GetEntries() )
    {
        auto write_thumbnail_cell = [&](const std::wstring& path)
        {
            html_writer << LR"(<td class="thumbnailSmall"><div class="thumbnailSmall"><img src=")"
                        << icon_png_provider->CreateUrl(path).c_str()
                        << LR"(" alt="")";

            if( PortableFunctions::FileIsRegular(path) )
            {
                const std::optional<std::wstring> mime_type = MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(path));

                if( mime_type.has_value() && MimeType::IsImageType(*mime_type) )
                {
                    html_writer << LR"( data-image=")"
                                << file_server.GetFilenameUrl(path).c_str()
                                << LR"(")";
                }
            }

            html_writer << LR"(></div></td>)";
        };

        auto write_path = [&](const TCHAR* full_path, const TCHAR* short_path)
        {
            std::wstring file_size = PortableFunctions::FileSizeString(full_path);

            if( !file_size.empty() )
                file_size = FormatTextCS2WS(_T("  (%s)"), file_size.c_str());

            auto write_path_and_file_size = [&](const TCHAR* path)
            {
                html_writer << path;

                if( !file_size.empty() )
                {
                    html_writer << LR"(<span class="fileSize">)"
                                << file_size
                                << LR"(</span>)";
                }
            };

            if( short_path == nullptr )
            {
                write_path_and_file_size(full_path);
            }

            else
            {
                html_writer << LR"(<span class="pathShort">)";
                write_path_and_file_size(short_path);
                html_writer << LR"(</span><span class="pathFull">)";
                write_path_and_file_size(full_path);
                html_writer << LR"(</span>)";
            }
        };

        html_writer << LR"(<tr class="mainInput">)";
        write_thumbnail_cell(pack_entry.GetPath().c_str());
        html_writer << LR"(<td colspan="2">)";
        write_path(pack_entry.GetPath().c_str(), nullptr);
        html_writer << LR"(</td></tr>)"
                       L"\n";

        for( const auto& [path, filename_for_display] : pack_entry.GetFilenamesForDisplay() )
        {
            html_writer << LR"(<tr class="dependentInput">)"
                           LR"(<td class="thumbnailSmall"></td>)";
            write_thumbnail_cell(path.c_str());
            html_writer << LR"(<td>)";
            write_path(path.c_str(), filename_for_display.c_str());
            html_writer << LR"(</td></tr>)"
                           L"\n";
        }
    }

    html_writer << ReportFooter;

    VirtualFileMapping virtual_file_mapping = file_server.CreateVirtualHtmlFile(GetTempDirectory(),
        [ html = UTF8Convert::WideToUTF8(html_writer.str()) ]()
        {
            return html;
        });

    HtmlViewDlg dlg;
    dlg.SetInitialUrl(virtual_file_mapping.GetUrl());
    dlg.DoModal();
}
