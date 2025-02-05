#pragma once


class Creator
{
public:
    Creator();
    ~Creator();

private:
    [[noreturn]] static void ThrowGitException();

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
