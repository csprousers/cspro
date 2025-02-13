#pragma once


// FullLineProcessor is a helper class that returns a character buffer that is of a minimum
// length; it may return the original buffer, or it may return a temporary buffer space-filled
// to be the minimum length
class FullLineProcessor
{
public:
    const TCHAR* GetLine(const TCHAR* source_line, size_t* source_length, size_t minimum_length)
    {
        if( minimum_length <= *source_length )
            return source_line;

        // allocate a buffer large enough to support the new line (and some more to reduce
        // multiple allocations)
        if( m_line.size() < minimum_length )
            m_line.resize(minimum_length * 2);

        TCHAR* line_data = m_line.data();

        // copy the source line and then right space pad it
        _tmemcpy(line_data, source_line, *source_length);
        _tmemset(line_data + *source_length, _T(' '), minimum_length - *source_length);

        *source_length = minimum_length;

        return line_data;
    }

    const TCHAR* GetLine(const TCHAR* source_line, size_t source_length, size_t minimum_length)
    {
        return GetLine(source_line, &source_length, minimum_length);
    }

private:
    std::vector<TCHAR> m_line;
};
