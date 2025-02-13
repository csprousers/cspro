#pragma once

//////////////////////////////////////////////////////////////////////////
//
// 3DException.h
//
// Exception classes definition

//
// The hierarchy defined here is the following
//
//                           GenericException
//                           /               \
//                  GenericNumberedException*  CIteratorException
//                          |        \
//                    FatalException  ExportException
//                          |
//                  CompilerTypeException
//                   /      |          \
//    LexerException  ParserException VarAnalysisException
//
// * = C3DException is an alias for GenericNumberedException
//
// rcl, March 11, 2005
//
//////////////////////////////////////////////////////////////////////////

class GenericException
{
public:
    GenericException() {}
    virtual ~GenericException() {}
};

#define EXTEND_GENERIC_EXCEPTION(new_x) \
class new_x : public GenericException \
{ public: new_x() {} virtual ~new_x() {} }

EXTEND_GENERIC_EXCEPTION(CIteratorException);

class GenericNumberedException : public GenericException
{
   int m_iError;
public:
    GenericNumberedException() : m_iError( 0 ) {}
    GenericNumberedException( int iError ) : m_iError( iError ) {}
    GenericNumberedException( const GenericNumberedException& other ) { m_iError = other.m_iError; }
    virtual ~GenericNumberedException() {}
    int getErrorCode() { return m_iError; }
};

// DEFINE_EXCEPTION_CLASS: macro to ease the class hierarchy definition,
//
// When using this macro, do not forget to use the ";" afterwards
// rcl, March 11, 2005
#define PRE_EXTRA_CODE
#define POST_EXTRA_CODE
#define DEFINE_EXCEPTION_CLASS(new_x, from_y) \
class new_x : public from_y { PRE_EXTRA_CODE \
                              public: new_x(int iError) : from_y(iError) {}\
                              POST_EXTRA_CODE }

// *now* it is easy
DEFINE_EXCEPTION_CLASS(        FatalException, GenericNumberedException );
DEFINE_EXCEPTION_CLASS(       ExportException, GenericNumberedException );

DEFINE_EXCEPTION_CLASS( CompilerTypeException, FatalException );
DEFINE_EXCEPTION_CLASS(        LexerException, CompilerTypeException );
DEFINE_EXCEPTION_CLASS(       ParserException, CompilerTypeException );
DEFINE_EXCEPTION_CLASS(  VarAnalysisException, CompilerTypeException );

// consts defined to make code in some files* clearer
//
// * = Expresc.cpp and Expresc_helper.cpp
constexpr int ERROR_INVALID_FUNCTION_CALL    =    14;
constexpr int ERROR_NO_DIMENSIONS_ALLOWED    =  8301;
constexpr int ERROR_TOO_MANY_SUBINDEXES      = 33003;
constexpr int ERROR_NOT_ENOUGH_SUBINDEXES    = 33010;
constexpr int ERROR_TOO_MANY_SUBINDEXES_UNIT = 33011;

constexpr int ERROR_POSITIVE_CONSTANT_EXPECTED = 5516;
constexpr int ERROR_RIGHT_PAREN_EXPECTED = 519;

#define C3DException GenericNumberedException
#define Fatal3DException FatalException
