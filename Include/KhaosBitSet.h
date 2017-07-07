#pragma once
#include "KhaosStdTypes.h"

namespace Khaos
{
#define KhaosBitSetFlag(t, i)   (t(1) << i)  
#define KhaosBitSetFlag8(i)     KhaosBitSetFlag(uint8, i)
#define KhaosBitSetFlag16(i)    KhaosBitSetFlag(uint16, i)
#define KhaosBitSetFlag32(i)    KhaosBitSetFlag(uint32, i)
#define KhaosBitSetFlag64(i)    KhaosBitSetFlag(uint64, i)

    template<typename T>
    class BitSet
    {
    public:
        enum { BitCount = sizeof(T) * 8 };
        typedef T ValueType;

    public:
        BitSet() : m_val(0) {}
        BitSet( T val ) : m_val(val) {}

    public:
        void setBit( int i )
        {
            m_val |= (T(1) << i);
        }

        void unsetBit( int i )
        {
            m_val &= ~(T(1) << i);
        }

        void enableBit( int i, bool en )
        {
            if ( en )
                setBit( i );
            else
                unsetBit( i );
        }

        void setFlag( T f )
        {
            m_val |= f;
        }

        void unsetFlag( T f )
        {
            m_val &= ~f;
        }

        void maskFlag( T f )
        {
            m_val &= f;
        }

        void mergeFlag( T f, T mask, int offset )
        {
            m_val |= ((f & mask) << offset);
        }

        void enableFlag( T f, bool en )
        {
            if ( en )
                setFlag( f );
            else
                unsetFlag( f );
        }

        T getMaskedValue( T f ) const
        {
            return m_val & f;
        }

        T fetchValue( T mask, int offset ) const
        {
            return (m_val >> offset) & mask;
        }

        bool testBit( int i ) const
        {
            return (m_val & (T(1) << i)) != 0;
        }

        bool testFlag( T f ) const
        {
            return (m_val & f) != 0;
        }

        void setAll()
        {
            m_val = T(-1);
        }

        void clear()
        {
            m_val = 0;
        }

        void setValue( T v )
        {
            m_val = v;
        }

        bool hasTest() const
        {
            return m_val != 0;
        }

        T getValue() const
        {
            return m_val;
        }

    public:
        bool operator==( const BitSet& rhs ) const
        {
            return m_val == rhs.m_val;
        }

        bool operator<( const BitSet& rhs ) const
        {
            return m_val < rhs.m_val;
        }

    private:
        T m_val;
    };

    typedef BitSet<uint8>   BitSet8;
    typedef BitSet<uint16>  BitSet16;
    typedef BitSet<uint32>  BitSet32;
    typedef BitSet<uint64>  BitSet64;
}

