#pragma once

#include <zUtilO/zUtilO.h>


namespace FileExtensions
{
#define C_EXT(VALUE_NAME, TEXT) /* C_EXT = Create Extension */                \
    constexpr const TCHAR* VALUE_NAME = TEXT;                                 \
    namespace WithDot  { constexpr const TCHAR* VALUE_NAME = _T(".") TEXT; }  \
    namespace Wildcard { constexpr const TCHAR* VALUE_NAME = _T("*.") TEXT; }

    // dictionary and application specs
    C_EXT(Dictionary, _T("dcf"))
    C_EXT(Form,       _T("fmf"))
    C_EXT(Order,      _T("ord"))
    C_EXT(TableSpec,  _T("xts"))

    // applications and their components
    C_EXT(EntryApplication,      _T("ent"))
    C_EXT(BatchApplication,      _T("bch"))
    C_EXT(TabulationApplication, _T("xtb"))
    C_EXT(Logic,                 _T("apc"))
    C_EXT(Message,               _T("mgf"))
    C_EXT(QuestionText,          _T("qsf"))
    C_EXT(ApplicationProperties, _T("csprops"))

    // runtime
    C_EXT(Listing,            _T("lst"))
    C_EXT(OperatorStatistics, _T("log"))
    C_EXT(Paradata,           _T("cslog"))
    C_EXT(Pff,                _T("pff"))

    // other
    C_EXT(AreaName,         _T("anm"))
    C_EXT(BinaryEntryPen,   _T("pen"))
    C_EXT(CHM,              _T("chm"))
    C_EXT(CSHTML,           _T("cshtml"))
    C_EXT(HTM,              _T("htm"))
    C_EXT(HTML,             _T("html"))
    C_EXT(JavaScript,       _T("js"))
    C_EXT(JavaScriptModule, _T("mjs"))
    C_EXT(Json,             _T("json"))
    C_EXT(PDF,              _T("pdf"))
    C_EXT(Pre77Report,      _T("csrs"))
    C_EXT(SaveArray,        _T("sva"))
    C_EXT(Table,            _T("tbw"))

    // tools
    C_EXT(CompareSpec,          _T("cmp"))
    C_EXT(DeploySpec,           _T("csds"))
    C_EXT(ExcelToCSProSpec,     _T("xl2cs"))
    C_EXT(ExportSpec,           _T("exf"))
    C_EXT(FrequencySpec,        _T("fqf"))
    C_EXT(PackSpec,             _T("cspack"))
    C_EXT(ProductionRunnerSpec, _T("pffRunner"))
    C_EXT(SortSpec,             _T("ssf"))

    // documentation
    C_EXT(CSDocument,    _T("csdoc"))
    C_EXT(CSDocumentSet, _T("csdocset"))

    // CSPro data
    namespace Data
    {
        C_EXT(CSProDB,            _T("csdb"))
        C_EXT(EncryptedCSProDB,   _T("csdbe"))
        C_EXT(IndexableTextIndex, _T("csidx"))
        C_EXT(Json,               _T("json"))
        C_EXT(TextDataDefault,    _T("dat"))
        C_EXT(TextNotes,          _T("csnot"))
        C_EXT(TextStatus,         _T("sts"))
    }

    // exported data
    C_EXT(CSV,                _T("csv"))
    C_EXT(Excel,              _T("xlsx"))
    C_EXT(RData,              _T("RData"))
    C_EXT(RSyntax,            _T("R"))
    C_EXT(SasData,            _T("xpt"))
    C_EXT(SasSyntax,          _T("sas"))
    C_EXT(SemicolonDelimited, _T("skv"))
    C_EXT(SpssData,           _T("sav"))
    C_EXT(SpssSyntax,         _T("sps"))
    C_EXT(StataData,          _T("dta"))
    C_EXT(StataDictionary,    _T("dct"))
    C_EXT(StataDo,            _T("do"))
    C_EXT(TabDelimited,       _T("tsv"))

    // binary tables
    namespace BinaryTable
    {
        C_EXT(Tab,      _T("tab"))
        C_EXT(TabIndex, _T("tabidx"))
        C_EXT(Tbd,      _T("tbd"))
        C_EXT(TbdIndex, _T("tbdidx"))
    }

    // previously used extensions
    namespace Old
    {
        C_EXT(BinaryEntryPen, _T("enc"))
        C_EXT(Logic,          _T("app"))

        namespace Data
        {
            C_EXT(TextIndex, _T("idx"))
            C_EXT(TextNotes, _T("not"))
        }
    }

#undef C_EXT


    // returns true if the filename has a HTML-related extension
    bool CLASS_DECL_ZUTILO IsFilenameHtml(wstring_view filename_sv);

    // returns true if the extension is disallowed for CSPro data files; the extension should not contain the dot
    bool CLASS_DECL_ZUTILO IsExtensionForbiddenForDataFiles(wstring_view extension_sv);
}


namespace FileFilters
{
    constexpr const TCHAR* Listing = _T("Listing Files (*.lst)|*.lst|HTML Files (*.html)|*.html|CSV Files (*.csv)|*.csv|All Files (*.*)|*.*||");

    constexpr const TCHAR* Pff     = _T("PFF Files (*.pff)|*.pff|All Files (*.*)|*.*||");
}
