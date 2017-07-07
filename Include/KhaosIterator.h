#pragma once

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    template<class T>
    class RangeIterator
    {
        typedef typename T::iterator  IteratorType;
        typedef typename T::reference ReferenceType;

    public:
        RangeIterator( T& container ) : m_it(container.begin()), m_itEnd(container.end()) {}

        RangeIterator( const T& container ) : 
            m_it(const_cast<T&>(container).begin()), m_itEnd(const_cast<T&>(container).end()) {}

        RangeIterator( IteratorType itBeg, IteratorType itEnd ) : m_it(itBeg), m_itEnd(itEnd) {}

    public:
        bool hasNext() const
        {
            return m_it != m_itEnd;
        }

        ReferenceType get()
        {
            return *m_it++;
        }

    private:
        IteratorType m_it;
        IteratorType m_itEnd;
    };

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    class RangeConstIterator
    {
        typedef typename T::const_iterator  IteratorType;
        typedef typename T::const_reference ReferenceType;

    public:
        RangeConstIterator( const T& container ) : m_it(container.begin()), m_itEnd(container.end()) {}

        RangeConstIterator( IteratorType itBeg, IteratorType itEnd ) : m_it(itBeg), m_itEnd(itEnd) {}

    public:
        bool hasNext() const
        {
            return m_it != m_itEnd;
        }

        ReferenceType get()
        {
            return *m_it++;
        }

    private:
        IteratorType m_it;
        IteratorType m_itEnd;
    };
}

