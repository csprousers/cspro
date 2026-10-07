#include "StdAfx.h"
#include "HelpTasks.h"
#include <zMultimediaO/ExifReader.h>


std::string HelpTasks::CreateFilePathForOutput(const char* const filename_suffix)
{
    return Path::Combine(GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop),
                         DateTime::LocalDateTimeString(DateTime::Now(), "%Y-%m-%d") + " - " + filename_suffix);
}


void HelpTasks::CreateExifTable()
{
    const std::string file_path = CreateFilePathForOutput("EXIF.txt");
    std::string output;

    ExifReader::ForeachTag(
        [&](const std::string& name, const char* const /*title*/, const char* const description, const std::vector<const char*>& ifds)
        {
            // tag name
            output.append("<cell nowrap><b>")
                  .append(Encoders::ToHtml(name, false))
                  .append("</b></cell>");

            // description
            output.append("<cell>");

            if( strlen(description) != 0 )
            {
                output.append(Encoders::ToHtml(description, false))
                      .append("~!~~!~");
            }

            output.append("<font Silver><i>IFDs: ");

            for( size_t i = 0; i < ifds.size(); ++i )
            {
                if( i > 0 )
                    output.append(", ");

                output.append(Encoders::ToHtml(ifds[i], false));
            }

            output.append("</i></font>~!~~!~&nbsp;</cell>\n");
        });

    FileIO::WriteText(file_path, output, false);
}
