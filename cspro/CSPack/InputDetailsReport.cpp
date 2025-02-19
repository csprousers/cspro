#include "StdAfx.h"
#include "PackDlg.h"
#include <zUtilO/MimeType.h>
#include <zHtml/HtmlViewDlg.h>
#include <zHtml/HtmlWriter.h>
#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zHtml/VirtualFileMapping.h>


namespace
{
    constexpr std::string_view ReportHeader_sv = R"(<!doctype html>
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

    constexpr std::string_view ReportFooter_sv = R"(
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
        bool ServeContent(VirtualFileMappingResponse& response, const std::string& key) override
        {
            std::shared_ptr<const std::vector<std::byte>> png_data = SystemIcon::GetPngForPath(key);

            if( png_data != nullptr )
            {
                response.SetContent(std::move(png_data), MimeType::Type::ImagePng);
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

    html_writer.WriteRaw(ReportHeader_sv);

    for( const PackEntry& pack_entry : m_packSpec->GetEntries() )
    {
        auto write_thumbnail_cell = [&](const std::string& path)
        {
            html_writer << R"(<td class="thumbnailSmall"><div class="thumbnailSmall"><img src=")";
            html_writer.WriteTagValue(icon_png_provider->CreateUrl(path))
                        << R"(" alt="")";

            if( PortableFunctions::FileIsRegular(path) )
            {
                const std::optional<std::string> mime_type = MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(path));

                if( mime_type.has_value() && MimeType::IsImageType(*mime_type) )
                {
                    html_writer << R"( data-image=")";
                    html_writer.WriteTagValue(file_server.CreateFileUrl(path))
                                << R"(")";
                }
            }

            html_writer << R"(></div></td>)";
        };

        auto write_path = [&](const std::string& full_path, const std::string* const short_path)
        {
            const std::string file_size = PortableFunctions::FileSizeString(full_path);

            auto write_path_and_file_size = [&](const std::string& path)
            {
                html_writer << path;

                if( !file_size.empty() )
                {
                    html_writer << R"(<span class="fileSize">)"
                                << "  (" << file_size << ")"
                                << R"(</span>)";
                }
            };

            if( short_path == nullptr )
            {
                write_path_and_file_size(full_path);
            }

            else
            {
                html_writer << R"(<span class="pathShort">)";
                write_path_and_file_size(*short_path);
                html_writer << R"(</span><span class="pathFull">)";
                write_path_and_file_size(full_path);
                html_writer << R"(</span>)";
            }
        };

        html_writer << R"(<tr class="mainInput">)";
        write_thumbnail_cell(pack_entry.GetPath());
        html_writer << R"(<td colspan="2">)";
        write_path(pack_entry.GetPath(), nullptr);
        html_writer << R"(</td></tr>)"
                       "\n";

        for( const auto& [path, filename_for_display] : pack_entry.GetFilenamesForDisplay() )
        {
            html_writer << R"(<tr class="dependentInput">)"
                           R"(<td class="thumbnailSmall"></td>)";
            write_thumbnail_cell(path);
            html_writer << R"(<td>)";
            write_path(path, &filename_for_display);
            html_writer << R"(</td></tr>)"
                           "\n";
        }
    }

    html_writer.WriteRaw(ReportFooter_sv);

    VirtualFileMapping virtual_file_mapping = file_server.CreateVirtualHtmlFile(GetTempDirectory(),
        [ html = SharableString(html_writer.str()) ]()
        {
            return html;
        });

    HtmlViewDlg dlg;
    dlg.SetInitialUrl(virtual_file_mapping.GetUrl());
    dlg.DoModal();
}
