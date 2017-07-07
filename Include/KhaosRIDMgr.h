#pragma once

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    class RIDObject : public AllocatedObject
    {
    public:
        RIDObject() : m_owner(0), m_rid(0) {}

    public:
        void  setOwner( void* o ) { m_owner = o; }
        void* getOwner() const { return m_owner; }

        void   setRID( uint32 id ) { m_rid = id; }
        uint32 getRID() const { return m_rid; }

    private:
        void*  m_owner;
        uint32 m_rid;
    };

    //////////////////////////////////////////////////////////////////////////
    class RIDObjectManager : public AllocatedObject
    {
        typedef vector<RIDObject*>::type RIDObjectList;
        typedef bool (*CmpFunc)( const RIDObject*, const RIDObject* );

    public:
        RIDObjectManager() : m_func(0), m_countUpdate(0), m_dirty(true) {}
        ~RIDObjectManager() { _clear(); }

    public:
        void setSortFunc( CmpFunc func ) { m_func = func; }

        RIDObject* requestRIDObj( void* owner );
        void freeRIDObj( RIDObject* rid );

        void setDirty() { m_dirty = true; }

        void update();

    private:
        void _clear();
        void _freeList( RIDObjectList& ridList );

    private:
        RIDObjectList m_ridObjs;
        RIDObjectList m_ridFrees;
        RIDObjectList m_ridTemp;
        CmpFunc       m_func;
        int           m_countUpdate;
        bool          m_dirty;
    };

   
}

