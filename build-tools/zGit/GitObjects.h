#pragma once

namespace Git { struct Tag; }


struct Git::Tag
{
    std::string id;
    std::string name;
};
