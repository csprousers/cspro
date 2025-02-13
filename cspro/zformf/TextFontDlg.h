#pragma once


/////////////////////////////////////////////////////////////////////////////
// CTextFontDialog dialog

class CTextFontDialog : public CFontDialog
{
public:
    using CFontDialog::CFontDialog;

    int DoModal() override;
};
