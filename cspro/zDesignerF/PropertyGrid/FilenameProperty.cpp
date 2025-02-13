#include "StdAfx.h"
#include "FilenamePropertyBase.h"


namespace PropertyGrid
{
    class FilenameProperty : public FilenamePropertyBase<Type::Filename>
    {
    public:
        FilenameProperty(std::shared_ptr<PropertyGridData<Type::Filename>> data)
            :   FilenamePropertyBase<Type::Filename>(data)
        {
        }
    };


    CMFCPropertyGridProperty* PropertyBuilder<Type::Filename>::ToProperty()
    {
        return new FilenameProperty(m_data);
    }
}
