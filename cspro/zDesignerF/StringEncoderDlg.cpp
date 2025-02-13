#include "StdAfx.h"
#include "StringEncoderDlg.h"
#include <zToolsO/RaiiHelpers.h>
#include <zJson/Json.h>
#include <zMessageO/ExceptionThrowingSystemMessageIssuer.h>
#include <zLogicO/BaseCompiler.h>
#include <zLogicO/SymbolTable.h>
#include <windowsx.h>


// --------------------------------------------------------------------------
// StringEncoderDlg::EncoderWorker framework
// --------------------------------------------------------------------------

class StringEncoderDlg::EncoderWorker
{
public:
    EncoderWorker(CLogicCtrl& logic_ctrl, int error_control_id)
        :   m_logicCtrl(logic_ctrl),
            m_errorCtrlId(error_control_id),
            m_errorHWnd(nullptr)
    {
    }

    virtual ~EncoderWorker() { }

    CLogicCtrl& GetLogicCtrl() { return m_logicCtrl; }

    int GetErrorControlId() const { return m_errorCtrlId; }

    HWND GetErrorHWnd() const    { return m_errorHWnd; }
    void SetErrorHWnd(HWND hWnd) { m_errorHWnd = hWnd; }

    virtual std::wstring GetPlainText(std::wstring text) = 0;
    virtual std::wstring GetEncodedText(const std::wstring& plain_text) = 0;

private:
    CLogicCtrl& m_logicCtrl;
    int m_errorCtrlId;
    HWND m_errorHWnd;
};


class TextEncoderWorker : public StringEncoderDlg::EncoderWorker
{
public:
    TextEncoderWorker(CLogicCtrl& logic_ctrl);
    
    std::wstring GetPlainText(std::wstring text) override;
    std::wstring GetEncodedText(const std::wstring& plain_text) override;    
};


class LogicEncoderWorker : public StringEncoderDlg::EncoderWorker
{
public:
    LogicEncoderWorker(CLogicCtrl& logic_ctrl, StringEncoderDlg& string_encoder_dlg, const LogicSettings& logic_settings);
    ~LogicEncoderWorker();

    std::wstring GetPlainText(std::wstring text) override;
    std::wstring GetEncodedText(const std::wstring& plain_text) override;

private:
    StringEncoderDlg& m_stringEncoderDlg;
    Logic::StringEscaper m_logicStringEscaper;

    class StringCompiler;
    std::unique_ptr<StringCompiler> m_stringCompiler;
};


class JsonEncoderWorker : public StringEncoderDlg::EncoderWorker
{
public:
    JsonEncoderWorker(CLogicCtrl& logic_ctrl, StringEncoderDlg& string_encoder_dlg);

    std::wstring GetPlainText(std::wstring text) override;
    std::wstring GetEncodedText(const std::wstring& plain_text) override;

private:
    StringEncoderDlg& m_stringEncoderDlg;
};


class PercentEncodingEncoderWorker : public StringEncoderDlg::EncoderWorker
{
public:
    PercentEncodingEncoderWorker(CLogicCtrl& logic_ctrl);

    std::wstring GetPlainText(std::wstring text) override;
    std::wstring GetEncodedText(const std::wstring& plain_text) override;
};



// --------------------------------------------------------------------------
// StringEncoderDlg
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(StringEncoderDlg, CDialog)
    ON_CONTROL_RANGE(EN_CHANGE, IDC_STRING_TEXT, IDC_STRING_PERCENT_ENCODING, OnTextChange)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_COPY_TEXT, IDC_COPY_PERCENT_ENCODING, OnCopy)
    ON_BN_CLICKED(IDC_SPLIT_NEWLINES, OnSplitNewlines)
    ON_BN_CLICKED(IDC_USE_VERBATIM_STRING_LITERALS, OnUseVerbatimStringLiterals)
    ON_BN_CLICKED(IDC_ESCAPE_FORWARD_SLASHES, OnEscapeForwardSlashes)
END_MESSAGE_MAP()


namespace
{
    constexpr COLORREF ErrorColor = RGB(255, 0, 0);
}


StringEncoderDlg::StringEncoderDlg(const LogicSettings& logic_settings, std::wstring initial_text, CWnd* pParent/* = nullptr*/)
    :   CDialog(StringEncoderDlg::IDD, pParent),
        m_lexerLanguage(Lexers::GetLexer_Logic(logic_settings)),
        m_initialText(std::move(initial_text)),
        m_escapeJsonForwardSlashes(WinSettings::Read<DWORD>(WinSettings::Type::CodeEscapeJsonForwardSlashes, 0) == 1),
        m_splitNewlinesHWnd(nullptr),
        m_useVerbatimStringLiteralsHWnd(nullptr),
        m_escapeJsonForwardSlashesHWnd(nullptr),
        m_currentErrorHWnd(nullptr),
        m_updatingText(false)
{
    // it is only possible to get newlines when escaping string literals
    if( logic_settings.EscapeStringLiterals() )
        m_splitNewlines = ( WinSettings::Read<DWORD>(WinSettings::Type::CodeLogicSplitNewlines, 1) == 1 );

    if( logic_settings.UseVerbatimStringLiterals() )
        m_useVerbatimStringLiterals = ( WinSettings::Read<DWORD>(WinSettings::Type::CodeLogicUseVerbatimStringLiterals, 0) == 1 );

    // set up the encoders
    m_encoderWorkers.emplace_back(std::make_unique<TextEncoderWorker>(m_textLogicCtrl));
    m_encoderWorkers.emplace_back(std::make_unique<LogicEncoderWorker>(m_logicLogicCtrl, *this, logic_settings));
    m_encoderWorkers.emplace_back(std::make_unique<JsonEncoderWorker>(m_jsonLogicCtrl, *this));
    m_encoderWorkers.emplace_back(std::make_unique<PercentEncodingEncoderWorker>(m_percentEncodingLogicCtrl));

    m_lastUpdatedEncoderWorker = m_encoderWorkers.front().get();
}


StringEncoderDlg::~StringEncoderDlg()
{
}


void StringEncoderDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_STRING_TEXT, m_textLogicCtrl);
    DDX_Control(pDX, IDC_STRING_LOGIC, m_logicLogicCtrl);
    DDX_Control(pDX, IDC_STRING_JSON, m_jsonLogicCtrl);
    DDX_Control(pDX, IDC_STRING_PERCENT_ENCODING, m_percentEncodingLogicCtrl);
}


BOOL StringEncoderDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_splitNewlinesHWnd = GetDlgItem(IDC_SPLIT_NEWLINES)->GetSafeHwnd();
    m_useVerbatimStringLiteralsHWnd = GetDlgItem(IDC_USE_VERBATIM_STRING_LITERALS)->GetSafeHwnd();
    m_escapeJsonForwardSlashesHWnd = GetDlgItem(IDC_ESCAPE_FORWARD_SLASHES)->GetSafeHwnd();

    // set up the logic controls
    m_textLogicCtrl.ReplaceCEdit(this, false, false, SCLEX_NULL);
    m_logicLogicCtrl.ReplaceCEdit(this, false, false, m_lexerLanguage);
    m_jsonLogicCtrl.ReplaceCEdit(this, false, false, SCLEX_JSON);
    m_percentEncodingLogicCtrl.ReplaceCEdit(this, false, false, SCLEX_PERCENT_ENCODING);

    // get handles to the error text controls
    for( EncoderWorker& encoder_worker : VI_V(m_encoderWorkers) )
    {
        int control_id = encoder_worker.GetErrorControlId();

        if( control_id != 0 )
            encoder_worker.SetErrorHWnd(GetDlgItem(control_id)->GetSafeHwnd());
    }

    // set the initial values
    Button_SetCheck(m_splitNewlinesHWnd, SplitNewlines() ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_useVerbatimStringLiteralsHWnd, UseVerbatimStringLiterals() ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_escapeJsonForwardSlashesHWnd, m_escapeJsonForwardSlashes ? BST_CHECKED : BST_UNCHECKED);

    if( !m_splitNewlines.has_value() )
        ::EnableWindow(m_splitNewlinesHWnd, FALSE);

    if( !m_useVerbatimStringLiterals.has_value() )
        ::EnableWindow(m_useVerbatimStringLiteralsHWnd, FALSE);

    UpdateText();

    m_textLogicCtrl.SetText(m_initialText);
    m_textLogicCtrl.SetFocus();

    return FALSE;
}


LRESULT StringEncoderDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = CDialog::WindowProc(message, wParam, lParam);

    // display the error messages in red
    if( message == WM_CTLCOLORSTATIC && std::any_of(m_encoderWorkers.cbegin(), m_encoderWorkers.cend(),
                                                    [hWnd = reinterpret_cast<HWND>(lParam)](const std::unique_ptr<EncoderWorker>& encoding_worker)
                                                    {
                                                        return ( encoding_worker->GetErrorHWnd() == hWnd );
                                                    }) )
    {
        HDC hDC = reinterpret_cast<HDC>(wParam);
        SetTextColor(hDC, ErrorColor);
    }

    return result;
}


void StringEncoderDlg::OnTextChange(UINT nID)
{
    if( m_updatingText )
        return;

    m_lastUpdatedEncoderWorker = m_encoderWorkers[nID - IDC_STRING_TEXT].get();

    UpdateText();
}


void StringEncoderDlg::OnCopy(UINT nID)
{
    EncoderWorker& encoding_worker = *m_encoderWorkers[nID - IDC_COPY_TEXT];

    WinClipboard::PutText(this, encoding_worker.GetLogicCtrl().GetText());
}


void StringEncoderDlg::OnSplitNewlines()
{
    ASSERT(m_splitNewlines.has_value());

    m_splitNewlines = ( Button_GetCheck(m_splitNewlinesHWnd) == BST_CHECKED );
    WinSettings::Write<DWORD>(WinSettings::Type::CodeLogicSplitNewlines, *m_splitNewlines);

    UpdateText();
}


void StringEncoderDlg::OnUseVerbatimStringLiterals()
{
    ASSERT(m_splitNewlines.has_value());

    m_useVerbatimStringLiterals = ( Button_GetCheck(m_useVerbatimStringLiteralsHWnd) == BST_CHECKED );
    WinSettings::Write<DWORD>(WinSettings::Type::CodeLogicUseVerbatimStringLiterals, *m_useVerbatimStringLiterals);

    UpdateText();
}


void StringEncoderDlg::OnEscapeForwardSlashes()
{
    m_escapeJsonForwardSlashes = ( Button_GetCheck(m_escapeJsonForwardSlashesHWnd) == BST_CHECKED );
    WinSettings::Write<DWORD>(WinSettings::Type::CodeEscapeJsonForwardSlashes, m_escapeJsonForwardSlashes);

    UpdateText();
}


void StringEncoderDlg::UpdateText()
{
    ASSERT(m_lastUpdatedEncoderWorker != nullptr && !m_updatingText);
    RAII::SetValueAndRestoreOnDestruction<bool> update_text_modifier(m_updatingText, true);

    // a routine to keep track of the displayed error message
    auto update_error_hwnd = [&](HWND hWnd)
    {
        if( m_currentErrorHWnd == hWnd )
            return;

        if( m_currentErrorHWnd != nullptr )
            ::ShowWindow(m_currentErrorHWnd, SW_HIDE);

        m_currentErrorHWnd = hWnd;

        if( m_currentErrorHWnd != nullptr )
            ::ShowWindow(m_currentErrorHWnd, SW_SHOW);
    };

    try
    {
        // get the plain text
        std::wstring plain_text = m_lastUpdatedEncoderWorker->GetPlainText(m_lastUpdatedEncoderWorker->GetLogicCtrl().GetText());

        // on success, clear any error message
        update_error_hwnd(nullptr);

        // update the other encoders
        for( EncoderWorker& encoder_worker : VI_V(m_encoderWorkers) )
        {
            if( &encoder_worker != m_lastUpdatedEncoderWorker )
                encoder_worker.GetLogicCtrl().SetText(encoder_worker.GetEncodedText(plain_text));
        }        
    }

    catch( const CSProException& exception )
    {
        // if there was an error decoding the text, show the error message
        ASSERT(m_lastUpdatedEncoderWorker->GetErrorHWnd() != nullptr);

        update_error_hwnd(m_lastUpdatedEncoderWorker->GetErrorHWnd());

        WindowsWS::SetWindowText(m_currentErrorHWnd, exception.GetErrorMessage());

        // ...and clear the text in the other locations
        for( EncoderWorker& encoder_worker : VI_V(m_encoderWorkers) )
        {
            if( &encoder_worker != m_lastUpdatedEncoderWorker )
                encoder_worker.GetLogicCtrl().SetText("");
        }
    }
}



// --------------------------------------------------------------------------
// TextEncoderWorker
// --------------------------------------------------------------------------

TextEncoderWorker::TextEncoderWorker(CLogicCtrl& logic_ctrl)
    :   EncoderWorker(logic_ctrl, 0)
{
}


std::wstring TextEncoderWorker::GetPlainText(std::wstring text)
{
    // only work with \n, not \r\n
    ASSERT(text.find('\r') == std::wstring::npos || text[text.find('\r') + 1] == '\n');

    SO::Remove(text, '\r');
    SO::MakeTrimRight(text, '\n');

    return text;
}


std::wstring TextEncoderWorker::GetEncodedText(const std::wstring& plain_text)
{
    return plain_text;
}



// --------------------------------------------------------------------------
// LogicEncoderWorker
// --------------------------------------------------------------------------

class LogicEncoderWorker::StringCompiler : public Logic::BaseCompiler
{
public:
    StringCompiler(const LogicSettings& logic_settings, std::unique_ptr<Logic::SymbolTable> dummy_symbol_table)
        :   Logic::BaseCompiler(*dummy_symbol_table),
            m_logicSettings(logic_settings),
            m_dummySymbolTable(std::move(dummy_symbol_table)),
            m_exceptionThrowingSystemMessageIssuer(std::make_unique<ExceptionThrowingSystemMessageIssuer>())
    {
    }

protected:
    const LogicSettings& GetLogicSettings() const override          { return m_logicSettings; }
    virtual const std::wstring& GetCurrentProcName() const override { return SO::EmptyString; }

    void FormatMessageAndProcessParserMessage(Logic::ParserMessage& parser_message, va_list parg) override
    {
        m_exceptionThrowingSystemMessageIssuer->IssueVA(parser_message, parg);
    }

public:
    std::wstring GetPlainText(std::wstring text)
    {
        SO::MakeTrim(text);

        if( text.empty() )
            throw CSProException("Missing start quote (\" or ')");

        auto source_buffer = std::make_unique<Logic::SourceBuffer>(std::move(text));
        source_buffer->Tokenize(m_logicSettings);

        SetSourceBuffer(std::move(source_buffer));

        std::wstring plain_text;

        while( true )
        {
            NextToken();

            if( Tkn == TOKEOP )
                return plain_text;

            if( Tkn != TOKSCTE )
                throw CSProException(_T("Valid CSPro logic but not a string literal: '%s'"), Tokstr.c_str());

            plain_text.append(Tokstr);
        }
    }

private:
    const LogicSettings& m_logicSettings;
    std::unique_ptr<Logic::SymbolTable> m_dummySymbolTable;
    std::unique_ptr<ExceptionThrowingSystemMessageIssuer> m_exceptionThrowingSystemMessageIssuer;
};


LogicEncoderWorker::LogicEncoderWorker(CLogicCtrl& logic_ctrl, StringEncoderDlg& string_encoder_dlg, const LogicSettings& logic_settings)
    :   EncoderWorker(logic_ctrl, IDC_ERROR_LOGIC),
        m_stringEncoderDlg(string_encoder_dlg),
        m_logicStringEscaper(logic_settings.EscapeStringLiterals()),
        m_stringCompiler(std::make_unique<StringCompiler>(logic_settings, std::make_unique<Logic::SymbolTable>()))
{
}


LogicEncoderWorker::~LogicEncoderWorker()
{
}


std::wstring LogicEncoderWorker::GetPlainText(std::wstring text)
{
    return m_stringCompiler->GetPlainText(std::move(text));
}


std::wstring LogicEncoderWorker::GetEncodedText(const std::wstring& plain_text)
{
    return m_stringEncoderDlg.SplitNewlines() ? m_logicStringEscaper.EscapeStringWithSplitNewlines(plain_text, m_stringEncoderDlg.UseVerbatimStringLiterals()) :
                                                m_logicStringEscaper.EscapeString(plain_text, m_stringEncoderDlg.UseVerbatimStringLiterals());
}



// --------------------------------------------------------------------------
// JsonEncoderWorker
// --------------------------------------------------------------------------

JsonEncoderWorker::JsonEncoderWorker(CLogicCtrl& logic_ctrl, StringEncoderDlg& string_encoder_dlg)
    :   EncoderWorker(logic_ctrl, IDC_ERROR_JSON),
        m_stringEncoderDlg(string_encoder_dlg)
{
}


std::wstring JsonEncoderWorker::GetPlainText(std::wstring text)
{
    auto json_node = Json::Parse(text);

    if( !json_node.IsString() )
        throw CSProException(_T("Valid JSON but not a string"));

    return json_node.Get<std::wstring>();
}


std::wstring JsonEncoderWorker::GetEncodedText(const std::wstring& plain_text)
{
    return Encoders::ToJsonString(plain_text, m_stringEncoderDlg.EscapeJsonForwardSlashes());
}



// --------------------------------------------------------------------------
// PercentEncodingEncoderWorker
// --------------------------------------------------------------------------

PercentEncodingEncoderWorker::PercentEncodingEncoderWorker(CLogicCtrl& logic_ctrl)
    :   EncoderWorker(logic_ctrl, IDC_ERROR_PERCENT_ENCODING)
{
}


std::wstring PercentEncodingEncoderWorker::GetPlainText(std::wstring text)
{
    for( size_t i = 0; i < text.size(); ++i )
    {
        TCHAR ch = text[i];

        // make sure all the escape sequences are correct
        if( ch == '%' )
        {
            if( ( i + 3 ) > text.length() )
            {
                throw CSProException(_T("Two hexadecimal characters must appear following the %% at position %d"),
                                     (int)i + 1);
            }

            if( _tcschr(Encoders::HexChars, std::towlower(text[i + 1])) == nullptr ||
                _tcschr(Encoders::HexChars, std::towlower(text[i + 2])) == nullptr )
            {
                throw CSProException(_T("The hexadecimal escape sequence '%s' is invalid at position %d"),
                                        text.substr(i, 3).c_str(), (int)i + 1);
            }

            i += 2;
        }

        // make sure that everything that must be escaped is escaped
        else if( !Encoders::IsPercentEncodingUnreservedCharacter(ch) )
        {
            throw CSProException(_T("The character '%c' is not an unreserved character and must be escaped to '%s' at position %d"),
                                    ch, Encoders::ToPercentEncoding(text.substr(i, 1)).c_str(), (int)i + 1);
        }
    }

    return Encoders::FromPercentEncoding(text);
}


std::wstring PercentEncodingEncoderWorker::GetEncodedText(const std::wstring& plain_text)
{
    return Encoders::ToPercentEncoding(plain_text);
}
