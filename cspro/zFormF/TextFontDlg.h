#pragma once


/////////////////////////////////////////////////////////////////////////////
// CTextFontDialog dialog

class CTextFontDialog : public CFontDialog
{
public:
    using CFontDialog::CFontDialog;

    INT_PTR DoModal() override;
};
