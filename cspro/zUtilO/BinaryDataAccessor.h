#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/BinaryData.h>

class BinaryDataReader;


// BinaryDataAccessor:
// this class wraps what should evaluate to a BinaryData object but
// allows for the lazy loading of binary data using a BinaryDataReader

// while the name of the BinaryDataReader class suggests that it only
// is for reading, even post-reading, it may be accessed by this
// class to notify of data changes


class CLASS_DECL_ZUTILO BinaryDataAccessor
{
public:
    // creates an object with potentially defined data
    BinaryDataAccessor(std::unique_ptr<BinaryData> binary_data = nullptr);

    // creates an object with defined binary data
    BinaryDataAccessor(BinaryData binary_data);

    // creates an object using a binary data reader;
    // the content and metadata will only be loaded on demand
    BinaryDataAccessor(std::shared_ptr<BinaryDataReader> binary_data_reader);

    // copy and move constructors
    BinaryDataAccessor(const BinaryDataAccessor& rhs);
    BinaryDataAccessor(BinaryDataAccessor&& rhs) = default;

    // returns true if the binary data is defined
    bool IsDefined() const;


    // resets the binary data to an undefined state
    void Reset();

    // clears the binary data, resetting the binary data to an undefined state;
    // this method is similar to Reset except that, if using a binary data reader,
    // it will be notified that the data changed
    void Clear();


    // returns binary data reader
    const BinaryDataReader* GetBinaryDataReader() const { return m_binaryDataReader.get(); }

    // returns the binary data, loading it using the binary data reader if necessary;
    // if using binary data reader, an exception might be thrown
    const BinaryData& GetBinaryData() const;

    // returns the binary data metadata, loading it using the binary data reader if necessary;
    // if using a binary data reader, an exception might be thrown
    const BinaryDataMetadata& GetBinaryDataMetadata() const;

    // returns the size of the binary data, querying it from the binary data reader if necessary;
    // if using a binary data reader, an exception might be thrown
    uint64_t GetBinaryDataSize() const;


    // sets the binary data;
    // if using a binary data reader, it will be notified that the data changed
    void SetBinaryData(BinaryData binary_data);

    template<typename... Args>
    void SetBinaryData(Args&&... args);

    // sets the binary data to be read using a binary data reader;
    // the content and metadata will only be loaded on demand;
    // if previously using a different binary data reader, it will be notified that the data
    // changed and the binary data reader will be modified to this new one
    void SetBinaryDataReader(std::shared_ptr<BinaryDataReader> binary_data_reader);

    // returns a non-const reference to the binary data metadata that allows for modifications;
    // if the binary data has not been loaded (when using a binary data reader), it will be loaded;
    // if using a binary data reader, or if trying to access undefined data, an exception might be thrown
    BinaryDataMetadata& GetBinaryDataMetadataForModification();


private:
    BinaryData& GetBinaryDataUsingBinaryDataReader();
    const BinaryDataMetadata& GetBinaryDataMetadataUsingBinaryDataReader();
    uint64_t GetBinaryDataSizeUsingBinaryDataReader();

private:
    std::unique_ptr<BinaryData> m_binaryData;
    std::shared_ptr<BinaryDataReader> m_binaryDataReader;
    bool m_binaryDataReaderRequiresQuerying;


public:
    std::shared_ptr<BinaryDataReader> GetSharedBinaryDataReaderIfNotQueried() const // BINARY_TYPES_TO_ENGINE_TODO remove as this is only used by Case::ApplyBinaryDataFor80
    {        
        return m_binaryDataReaderRequiresQuerying ? m_binaryDataReader :
                                                    nullptr;
    } 
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline BinaryDataAccessor::BinaryDataAccessor(std::unique_ptr<BinaryData> binary_data/* = nullptr*/)
    :   m_binaryData(std::move(binary_data)),
        m_binaryDataReaderRequiresQuerying(false)
{
}


inline BinaryDataAccessor::BinaryDataAccessor(BinaryData binary_data)
    :   BinaryDataAccessor(std::make_unique<BinaryData>(std::move(binary_data)))
{
}


inline BinaryDataAccessor::BinaryDataAccessor(std::shared_ptr<BinaryDataReader> binary_data_reader)
    :   m_binaryDataReader(std::move(binary_data_reader)),
        m_binaryDataReaderRequiresQuerying(true)
{
    ASSERT(m_binaryDataReader != nullptr);
}


inline bool BinaryDataAccessor::IsDefined() const
{
    return ( m_binaryData != nullptr || m_binaryDataReaderRequiresQuerying );
}


inline void BinaryDataAccessor::Reset()
{
    m_binaryData.reset();
    m_binaryDataReader.reset();
    m_binaryDataReaderRequiresQuerying = false;
}


inline const BinaryData& BinaryDataAccessor::GetBinaryData() const
{
    return ( m_binaryData != nullptr ) ? *m_binaryData :
                                         const_cast<BinaryDataAccessor*>(this)->GetBinaryDataUsingBinaryDataReader();
}


inline const BinaryDataMetadata& BinaryDataAccessor::GetBinaryDataMetadata() const
{
    return ( m_binaryData != nullptr ) ? m_binaryData->GetMetadata() :
                                         const_cast<BinaryDataAccessor*>(this)->GetBinaryDataMetadataUsingBinaryDataReader();
}


inline uint64_t BinaryDataAccessor::GetBinaryDataSize() const
{
    return ( m_binaryData != nullptr ) ? m_binaryData->GetContent().size() :
                                         const_cast<BinaryDataAccessor*>(this)->GetBinaryDataSizeUsingBinaryDataReader();
}


template<typename... Args>
void BinaryDataAccessor::SetBinaryData(Args&&... args)
{
    SetBinaryData(BinaryData(std::forward<Args>(args)...));
}


inline BinaryDataMetadata& BinaryDataAccessor::GetBinaryDataMetadataForModification()
{
    return ( m_binaryData != nullptr ) ? m_binaryData->GetMetadata() :
                                         GetBinaryDataUsingBinaryDataReader().GetMetadata();
}
