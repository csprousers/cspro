#pragma once

class CEngineCompFunc;
class CEngineDriver;


class CompilerCreator
{
public:
    virtual ~CompilerCreator() { }

    virtual std::unique_ptr<CEngineCompFunc> CreateCompiler(CEngineDriver* pEngineDriver) = 0;
};
