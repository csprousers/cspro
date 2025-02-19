#pragma once

#include <zJavaScript/QuickJSAccess.h>

namespace JavaScript { class Atom; }


// --------------------------------------------------------------------------
// JavaScript::Atom is a wrapper around JSAtom values, where the atom is
// freed on destruction.
// --------------------------------------------------------------------------

class JavaScript::Atom
{
public:
    Atom(QuickJSAccess& qjs, JSAtom&& js_atom);
    Atom(QuickJSAccess& qjs, std::string_view text_sv);

    Atom(const Atom& rhs_atom);
    Atom(Atom&& rhs_atom) noexcept;

    Atom& operator=(const Atom& rhs_atom);
    Atom& operator=(Atom&& rhs_atom) noexcept;

    ~Atom();

    const JSAtom& GetAtom() const { return m_atom; }
    JSAtom& GetAtom()             { return m_atom; }

    const JSAtom& operator*() const { return m_atom; }
    JSAtom& operator*()             { return m_atom; }

private:
    QuickJSAccess* m_qjs;
    JSAtom m_atom;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline JavaScript::Atom::Atom(QuickJSAccess& qjs, JSAtom&& js_atom)
    :   m_qjs(&qjs),
        m_atom(std::move(js_atom))
{
}


inline JavaScript::Atom::Atom(QuickJSAccess& qjs, const std::string_view text_sv)
    :   m_qjs(&qjs),
        m_atom(m_qjs->NewAtom(text_sv))
{
}


inline JavaScript::Atom::Atom(const Atom& rhs_atom)
    :   m_qjs(rhs_atom.m_qjs),
        m_atom(JS_DupAtom(m_qjs->ctx, rhs_atom.m_atom))
{
    ASSERT(rhs_atom.m_qjs != nullptr);
}


inline JavaScript::Atom::Atom(Atom&& rhs_atom) noexcept
    :   m_qjs(rhs_atom.m_qjs),
        m_atom(rhs_atom.m_atom)
{
    ASSERT(rhs_atom.m_qjs != nullptr);
    rhs_atom.m_qjs = nullptr;
}


inline JavaScript::Atom& JavaScript::Atom::operator=(const Atom& rhs_atom)
{
    ASSERT(rhs_atom.m_qjs != nullptr);
    m_qjs = rhs_atom.m_qjs;
    m_atom = JS_DupAtom(m_qjs->ctx, m_atom);
    return *this;
}


inline JavaScript::Atom& JavaScript::Atom::operator=(Atom&& rhs_atom) noexcept
{
    ASSERT(rhs_atom.m_qjs != nullptr);
    m_qjs = rhs_atom.m_qjs;
    m_atom = rhs_atom.m_atom;
    rhs_atom.m_qjs = nullptr;
    return *this;
}


inline JavaScript::Atom::~Atom()
{
    if( m_qjs != nullptr )
        JS_FreeAtom(m_qjs->ctx, m_atom);
}
