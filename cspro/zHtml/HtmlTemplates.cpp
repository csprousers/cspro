#include "stdafx.h"
#include "HtmlTemplates.h"
#include "HtmlWriter.h"


std::string HtmlTemplates::CreateCenteredTextPage(const std::string_view title_sv, const std::string_view text_sv)
{
    constexpr std::string_view Html1_sv =
R"(<meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)";

    constexpr std::string_view Html2_sv =
R"(</title>
    <style>
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }
    </style>
</head>
<body>
    <div>)";

    constexpr std::string_view Html3_sv =
R"(</div>
</body>
</html>)";

    return SO::Concatenate(HtmlWriter::DefaultHeader_sv,
                           Html1_sv,
                           Encoders::ToHtml(title_sv),
                           Html2_sv,
                           Encoders::ToHtml(text_sv),
                           Html3_sv);
}
