#pragma once
#include "KhaosStdTypes.h"
#include "KhaosStringUtil.h"

namespace Khaos
{
    class AutoNameManager
    {
    public:
        AutoNameManager() : m_autoNameIndex(0) {}
        ~AutoNameManager() {}

    protected:
        int _getNextId()
        {
            int id = m_autoNameIndex;
            ++m_autoNameIndex;
            return id;
        }

        String _getNextName( const String& baseName )
        {
            return baseName + intToString( _getNextId() );
        }

        String _getNextName()
        {
            static const String baseName = "_auto_";
            return _getNextName( baseName );
        }

#define _checkNextName(name) ((name).size() ? (name) : _getNextName())

    protected:
        int m_autoNameIndex;
    };
}

