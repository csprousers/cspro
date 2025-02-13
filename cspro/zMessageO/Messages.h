#pragma once

#include <zMessageO/zMessageO.h>


namespace MGF
{
    ZMESSAGEO_API const std::wstring& GetMessageText(int message_number);

    ZMESSAGEO_API std::wstring GetMessageText(int message_number, const TCHAR* default_text);

    constexpr int OutOfRangeConfirm                 =  88888;
    constexpr int OutOfRange                        =  88889;

    constexpr int OutOfRangeOperatorControlledTitle =  89201;
    constexpr int OutOfSequenceTitle                =  89202;
    constexpr int EnterValidValue                   =  89203;
    constexpr int EnterValidValueConfirm            =  89204;
    constexpr int EnterCorrectValue                 =  89205;
    constexpr int PressF8ToClear                    =  89206;
                                                   
    constexpr int GotoItemNotFound                  =  89211;
    constexpr int GotoInvalidOccurrence             =  89212;
                                                   
    // Partial Save                                
    constexpr int PartialSaveTitle                  =  89221;
    constexpr int PartialSaveGotoLastPosition       =  89222;
    constexpr int PartialSaveCannotModify           =  89223;
    constexpr int PartialSaveSuccess                =  89224;
    constexpr int PartialSaveFailure                =  89225;
                                                   
    constexpr int InteractiveEditCannotOpenPartial  =  89231;
    constexpr int InteractiveEditCancel             =  89232;
                                                   
    constexpr int VerifyDone                        =  89241;
    constexpr int VerifyFieldNotMatch               =  89242;
    constexpr int VerifyReenter                     =  89243;
                                                   
    constexpr int DiscardQuestionnaire              =  89251;
    constexpr int DeleteNode                        =  89252;
    constexpr int DeleteCase                        =  89253;
                                                   
    constexpr int OccurrenceDoesNotExist            =  89261;
    constexpr int CreateNewFile                     =  89263;
                                                   
    constexpr int ErrorStartModify                  =  89271;
    constexpr int ErrorApplicationMode              =  89272;
    constexpr int ErrorReadingPen                   =  89273;
    constexpr int ErrorEngine                       =  89274;
                                                   
    constexpr int IndexCannotCreate                 =  89291;
    constexpr int IndexDuplicate                    =  89292;

    constexpr int OperatorEnteredInvalidValue       =  99990;

    // Portable Messages
    constexpr int Ok                                = 110001;
    constexpr int Cancel                            = 110002;
    constexpr int Yes                               = 110003;
    constexpr int No                                = 110004;

    constexpr int StopAddingTitle                   = 110041;
    constexpr int StopModifyingTitle                = 110042;
    constexpr int SaveChanges                       = 110043;
    constexpr int DiscardChanges                    = 110044;
    constexpr int AdvanceToEnd                      = 110045;

    constexpr int SystemErrorTitle                  = 110011;
    constexpr int SystemWarningTitle                = 110012;
    constexpr int UserErrorTitle                    = 110013;
    constexpr int AbortTitle                        = 110014;

    constexpr int SelectLanguageTitle               = 110051;
    constexpr int SelectLanguageOnlyOneDefined      = 110052;

    constexpr int InvalidMessageNumber              = 110061;
    constexpr int InvalidMessageParameterNumber     = 110062;
    constexpr int InvalidMessageParameterCount      = 110063;
}
