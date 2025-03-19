#pragma once

#include <zUtilO/zUtilO.h>


// --------------------------------------------------------------------------
// FileExtensions
// --------------------------------------------------------------------------

namespace FileExtensions
{
    // C_EXT = Create Extension
    #define C_EXT(VALUE_NAME, TEXT) constexpr const char* VALUE_NAME = TEXT;

    // dictionary and application specs
    C_EXT(Dictionary, "dcf")
    C_EXT(Form,       "fmf")
    C_EXT(Order,      "ord")
    C_EXT(TableSpec,  "xts")

    // applications and their components
    C_EXT(EntryApplication,      "ent")
    C_EXT(BatchApplication,      "bch")
    C_EXT(TabulationApplication, "xtb")
    C_EXT(Logic,                 "apc")
    C_EXT(Message,               "mgf")
    C_EXT(QuestionText,          "qsf")
    C_EXT(ApplicationProperties, "csprops")

    // runtime
    C_EXT(Listing,            "lst")
    C_EXT(OperatorStatistics, "log")
    C_EXT(Paradata,           "cslog")
    C_EXT(Pff,                "pff")

    // other
    C_EXT(AreaName,         "anm")
    C_EXT(BinaryEntryPen,   "pen")
    C_EXT(CHM,              "chm")
    C_EXT(CSHTML,           "cshtml")
    C_EXT(HTM,              "htm")
    C_EXT(HTML,             "html")
    C_EXT(JavaScript,       "js")
    C_EXT(JavaScriptModule, "mjs")
    C_EXT(Json,             "json")
    C_EXT(Markdown,         "md")
    C_EXT(PDF,              "pdf")
    C_EXT(Pre77Report,      "csrs")
    C_EXT(SaveArray,        "sva")
    C_EXT(Table,            "tbw")
    C_EXT(Text,             "txt")
    C_EXT(Zip,              "zip")

    // tools
    C_EXT(CompareSpec,          "cmp")
    C_EXT(DeploySpec,           "csds")
    C_EXT(ExcelToCSProSpec,     "xl2cs")
    C_EXT(ExportSpec,           "exf")
    C_EXT(FrequencySpec,        "fqf")
    C_EXT(PackSpec,             "cspack")
    C_EXT(ProductionRunnerSpec, "pffRunner")
    C_EXT(SortSpec,             "ssf")

    // documentation
    C_EXT(CSDocument,    "csdoc")
    C_EXT(CSDocumentSet, "csdocset")

    // CSPro data
    namespace Data
    {
        C_EXT(CSProDB,            "csdb")
        C_EXT(EncryptedCSProDB,   "csdbe")
        C_EXT(IndexableTextIndex, "csidx")
        C_EXT(Json,               "json")
        C_EXT(TextDataDefault,    "dat")
        C_EXT(TextNotes,          "csnot")
        C_EXT(TextStatus,         "sts")
    }

    // exported data
    C_EXT(CSV,                "csv")
    C_EXT(Excel,              "xlsx")
    C_EXT(RData,              "RData")
    C_EXT(RSyntax,            "R")
    C_EXT(SasData,            "xpt")
    C_EXT(SasSyntax,          "sas")
    C_EXT(SemicolonDelimited, "skv")
    C_EXT(SpssData,           "sav")
    C_EXT(SpssSyntax,         "sps")
    C_EXT(StataData,          "dta")
    C_EXT(StataDictionary,    "dct")
    C_EXT(StataDo,            "do")
    C_EXT(TabDelimited,       "tsv")

    // binary tables
    namespace BinaryTable
    {
        C_EXT(Tab,      "tab")
        C_EXT(TabIndex, "tabidx")
        C_EXT(Tbd,      "tbd")
        C_EXT(TbdIndex, "tbdidx")
    }

    // previously used extensions
    namespace Old
    {
        C_EXT(BinaryEntryPen, "enc")
        C_EXT(Logic,          "app")

        namespace Data
        {
            C_EXT(TextIndex, "idx")
            C_EXT(TextNotes, "not")
        }
    }

#undef C_EXT


    // Returns true if the extension matches a list of HTML-related extensions.
    bool CLASS_DECL_ZUTILO IsExtensionHtml(std::string_view extension_sv);

    // Returns true if the extension, calculated from the filename, has a HTML-related extension.
    bool CLASS_DECL_ZUTILO IsFileHtml(std::string_view filename_sv);

    // Returns true if the extension, calculated from the filename, is generally associated with compressed data.
    bool CLASS_DECL_ZUTILO IsFileCompressedData(std::string_view filename_sv);

    // Returns true if the extension is disallowed for CSPro data files.
    // The extension should not contain the dot.
    bool CLASS_DECL_ZUTILO IsExtensionForbiddenForDataFiles(std::string_view extension_sv);

    // Creates a string with a dot followed by an extension.
    // The extension should not contain the dot.
    inline std::string WithDot(cs::string_sz extension) { ASSERT(*extension.c_str() != '.'); return std::string(".").append(extension.c_str()); }

    // Creates a wildcard from an extension.
    // The extension should not contain the dot.
    inline std::string CreateWildcard(cs::string_sz extension) { ASSERT(*extension.c_str() != '.'); return std::string("*.").append(extension.c_str()); }
}


// --------------------------------------------------------------------------
// FileFilters
// --------------------------------------------------------------------------

namespace FileFilters
{
    constexpr const char* Dictionary = "Data Dictionary Files (*.dcf)|*.dcf|All Files (*.*)|*.*||";
    constexpr const char* HTML       = "HTML Files (*.html)|*.html|All Files (*.*)|*.*||";
    constexpr const char* Listing    = "Listing Files (*.lst)|*.lst|HTML Files (*.html)|*.html|CSV Files (*.csv)|*.csv|All Files (*.*)|*.*||";
    constexpr const char* Pff        = "PFF Files (*.pff)|*.pff|All Files (*.*)|*.*||";
    constexpr const char* Text       = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";
}


// --------------------------------------------------------------------------
// FileExtensionAnalyzer
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO FileExtensionAnalyzer
{
public:
    FileExtensionAnalyzer(std::string_view filename_sv);

    bool IsTypeHtml() const            { return ( m_type == Type::Html ); }
    bool IsTypeMarkdown() const        { return ( m_type == Type::Markdown ); }
    bool IsTypeHtmlOrDerivable() const { return ( m_type != Type::None ); }

private:
    enum class Type { None, Html, Markdown };
    Type m_type;
};
