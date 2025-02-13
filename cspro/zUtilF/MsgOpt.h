#pragma once

#include <zMessageO/MessageType.h>


class CMsgOptions
{
public:
    CMsgOptions(const CString& csTitle, const CString& csText, UINT nType, int iFocusButton, int iCancelReturn,
                const std::vector<CString>& aButtons = std::vector<CString>(),
                std::optional<MessageType> message_type = std::nullopt,
                std::optional<int> message_number = std::nullopt)
        :   m_csTitle(csTitle),
            m_csMsg(csText),
            m_nType(nType),
            m_iFocusButton(iFocusButton),
            m_iCancelReturn(iCancelReturn),
            m_aButtons(aButtons),
            m_messageType(std::move(message_type)),
            m_messageNumber(std::move(message_number))
    {
    }

    const CString& GetTitle() const                          { return m_csTitle; }
    const CString& GetMsg() const                            { return m_csMsg; }
    UINT GetType() const                                     { return m_nType; }
    int GetFocusButton() const                               { return m_iFocusButton; }
    int GetCancelReturn() const                              { return m_iCancelReturn; }
    const std::vector<CString>& GetArrayButtons() const      { return m_aButtons; }
    const std::optional<MessageType>& GetMessageType() const { return m_messageType; }
    const std::optional<int>& GetMessageNumber() const       { return m_messageNumber; }

private:
    CString m_csTitle;
    CString m_csMsg;
    UINT m_nType;        // Same as AfxMessageBox
    int m_iFocusButton;  // Zero base. -1 No focus
    int m_iCancelReturn; // When Close the dialog by closing the windows or ESC.
    const std::vector<CString> m_aButtons;
    std::optional<MessageType> m_messageType;
    std::optional<int> m_messageNumber;
};
