#pragma once

class Case;


// An Export Writer is created by using the ExportWriterRepository.

class ExportWriter
{
public:
    virtual ~ExportWriter() { }

    // Writes the case data.
    virtual void WriteCase(const Case& data_case) = 0;

    // Closes the export. The validity of the exported data can only be assured if the export is closed.
    virtual void Close() = 0;
};
