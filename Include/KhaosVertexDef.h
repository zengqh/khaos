#pragma once
#include "KhaosStdTypes.h"
#include "KhaosVector3.h"
#include "KhaosVector2.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    // KHAOS_MAKE_VERTEX_STRUCT_n
    enum VertexFixedMask
    {
        VFM_POS  = 0x1,
        VFM_NOR  = 0x2,
        VFM_TEX  = 0x4,
        VFM_CLR  = 0x8,
        VFM_TAN  = 0x10,
        VFM_TEX2 = 0x20,

        VFM_BONI = 0x40,
        VFM_BONW = 0x800,
        
        VFM_SH0  = 0x80,
        VFM_SH1  = 0x100,
        VFM_SH2  = 0x200,
        VFM_SH3  = 0x400
    };

#define KHAOS_VERTEX_MEMBER_POS     float x, y, z; const Vector3& pos() const { return *(Vector3*)&x; } Vector3& pos() { return *(Vector3*)&x; }
#define KHAOS_VERTEX_MEMBER_NOR     float nx, ny, nz; const Vector3& norm() const { return *(Vector3*)&nx; } Vector3& norm() { return *(Vector3*)&nx; }
#define KHAOS_VERTEX_MEMBER_TEX     float u, v; const Vector2& tex() const { return *(Vector2*)&u; } Vector2& tex() { return *(Vector2*)&u; }
#define KHAOS_VERTEX_MEMBER_CLR     uint32 clr;
#define KHAOS_VERTEX_MEMBER_TAN     float tx, ty, tz, tw; const Vector4& tang() const { return *(Vector4*)&tx; } Vector4& tang() { return *(Vector4*)&tx; }
#define KHAOS_VERTEX_MEMBER_TEX2    float u2, v2; const Vector2& tex2() const { return *(Vector2*)&u2; } Vector2& tex2() { return *(Vector2*)&u2; }
#define KHAOS_VERTEX_MEMBER_BONI    uint8 bi[4];
#define KHAOS_VERTEX_MEMBER_BONW    float bw[4];

#define KHAOS_VERTEX_MEMBER_SH0     float sh0, sh1, sh2, sh3;
#define KHAOS_VERTEX_MEMBER_SH1     float sh4, sh5, sh6, sh7;
#define KHAOS_VERTEX_MEMBER_SH2     float sh8, sh9, sh10, sh11;
#define KHAOS_VERTEX_MEMBER_SH3     float sh12, sh13, sh14, sh15;

#define KHAOS_VERTEX_MEMBER(x)      KHAOS_VERTEX_MEMBER_##x

#define KHAOS_MAKE_VERTEX_STRUCT_1(name, a) \
    struct Vertex_##a \
    { \
        enum { ID = VFM_##a }; \
        KHAOS_VERTEX_MEMBER(a); \
    }; \
    typedef Vertex_##a name;

#define KHAOS_MAKE_VERTEX_STRUCT_2(name, a, b) \
    struct Vertex_##a##b \
    { \
        enum { ID = VFM_##a | VFM_##b }; \
        KHAOS_VERTEX_MEMBER(a); \
        KHAOS_VERTEX_MEMBER(b); \
    }; \
    typedef Vertex_##a##b name;

#define KHAOS_MAKE_VERTEX_STRUCT_3(name, a, b, c) \
    struct Vertex_##a##b##c \
    { \
        enum { ID = VFM_##a | VFM_##b | VFM_##c }; \
        KHAOS_VERTEX_MEMBER(a); \
        KHAOS_VERTEX_MEMBER(b); \
        KHAOS_VERTEX_MEMBER(c); \
    }; \
    typedef Vertex_##a##b##c name;

#define KHAOS_MAKE_VERTEX_STRUCT_4(name, a, b, c, d) \
    struct Vertex_##a##b##c##d \
    { \
        enum { ID = VFM_##a | VFM_##b | VFM_##c | VFM_##d }; \
        KHAOS_VERTEX_MEMBER(a); \
        KHAOS_VERTEX_MEMBER(b); \
        KHAOS_VERTEX_MEMBER(c); \
        KHAOS_VERTEX_MEMBER(d); \
    }; \
    typedef Vertex_##a##b##c##d name;

#define KHAOS_MAKE_VERTEX_STRUCT_5(name, a, b, c, d, e) \
    struct Vertex_##a##b##c##d##e \
    { \
        enum { ID = VFM_##a | VFM_##b | VFM_##c | VFM_##d | VFM_##e }; \
        KHAOS_VERTEX_MEMBER(a); \
        KHAOS_VERTEX_MEMBER(b); \
        KHAOS_VERTEX_MEMBER(c); \
        KHAOS_VERTEX_MEMBER(d); \
        KHAOS_VERTEX_MEMBER(e); \
    }; \
    typedef Vertex_##a##b##c##d##e name;

#define KHAOS_MAKE_VERTEX_STRUCT_6(name, a, b, c, d, e, f) \
    struct Vertex_##a##b##c##d##e##f \
    { \
        enum { ID = VFM_##a | VFM_##b | VFM_##c | VFM_##d | VFM_##e | VFM_##f }; \
        KHAOS_VERTEX_MEMBER(a); \
        KHAOS_VERTEX_MEMBER(b); \
        KHAOS_VERTEX_MEMBER(c); \
        KHAOS_VERTEX_MEMBER(d); \
        KHAOS_VERTEX_MEMBER(e); \
        KHAOS_VERTEX_MEMBER(f); \
    }; \
    typedef Vertex_##a##b##c##d##e##f name;

#define KHAOS_MAKE_VERTEX_STRUCT_7(name, a, b, c, d, e, f, g) \
    struct Vertex_##a##b##c##d##e##f##g \
    { \
        enum { ID = VFM_##a | VFM_##b | VFM_##c | VFM_##d | VFM_##e | VFM_##f | VFM_##g }; \
        KHAOS_VERTEX_MEMBER(a); \
        KHAOS_VERTEX_MEMBER(b); \
        KHAOS_VERTEX_MEMBER(c); \
        KHAOS_VERTEX_MEMBER(d); \
        KHAOS_VERTEX_MEMBER(e); \
        KHAOS_VERTEX_MEMBER(f); \
        KHAOS_VERTEX_MEMBER(g); \
    }; \
    typedef Vertex_##a##b##c##d##e##f##g name;

#define KHAOS_MAKE_VERTEX_STRUCT_8(name, a, b, c, d, e, f, g, h) \
    struct Vertex_##a##b##c##d##e##f##g##h \
    { \
        enum { ID = VFM_##a | VFM_##b | VFM_##c | VFM_##d | VFM_##e | VFM_##f | VFM_##g | VFM_##h }; \
        KHAOS_VERTEX_MEMBER(a); \
        KHAOS_VERTEX_MEMBER(b); \
        KHAOS_VERTEX_MEMBER(c); \
        KHAOS_VERTEX_MEMBER(d); \
        KHAOS_VERTEX_MEMBER(e); \
        KHAOS_VERTEX_MEMBER(f); \
        KHAOS_VERTEX_MEMBER(g); \
        KHAOS_VERTEX_MEMBER(h); \
    }; \
    typedef Vertex_##a##b##c##d##e##f##g##h name;

    //////////////////////////////////////////////////////////////////////////
    // KHAOS_MAKE_VERTEX_DECL_n

#define KHAOS_NEXT_ELEMENT_POS(vtx)  vd->nextElement( VFM_POS, VES_POSITION, VET_FLOAT3, KHAOS_MEMBER_OFFSET(vtx, x), 0 )
#define KHAOS_NEXT_ELEMENT_NOR(vtx)  vd->nextElement( VFM_NOR, VES_NORMAL, VET_FLOAT3, KHAOS_MEMBER_OFFSET(vtx, nx), 0 )
#define KHAOS_NEXT_ELEMENT_TEX(vtx)  vd->nextElement( VFM_TEX, VES_TEXTURE, VET_FLOAT2, KHAOS_MEMBER_OFFSET(vtx, u), 0 )
#define KHAOS_NEXT_ELEMENT_CLR(vtx)  vd->nextElement( VFM_CLR, VES_COLOR, VET_COLOR, KHAOS_MEMBER_OFFSET(vtx, clr), 0 )
#define KHAOS_NEXT_ELEMENT_TAN(vtx)  vd->nextElement( VFM_TAN, VES_TANGENT, VET_FLOAT4, KHAOS_MEMBER_OFFSET(vtx, tx), 0 )
#define KHAOS_NEXT_ELEMENT_TEX2(vtx) vd->nextElement( VFM_TEX2, VES_TEXTURE, VET_FLOAT2, KHAOS_MEMBER_OFFSET(vtx, u2), 1 )

#define KHAOS_NEXT_ELEMENT_BONI(vtx) vd->nextElement( VFM_BONI, VES_BONEINDICES, VET_UBYTE4, KHAOS_MEMBER_OFFSET(vtx, bi), 0 )
#define KHAOS_NEXT_ELEMENT_BONW(vtx) vd->nextElement( VFM_BONW, VES_BONEWEIGHT, VET_FLOAT4, KHAOS_MEMBER_OFFSET(vtx, bw), 0 )

#define KHAOS_NEXT_ELEMENT_SH0(vtx)  vd->nextElement( VFM_SH0, VES_BONEWEIGHT, VET_FLOAT4, KHAOS_MEMBER_OFFSET(vtx, sh0), 1 )
#define KHAOS_NEXT_ELEMENT_SH1(vtx)  vd->nextElement( VFM_SH1, VES_BONEWEIGHT, VET_FLOAT4, KHAOS_MEMBER_OFFSET(vtx, sh4), 2 )
#define KHAOS_NEXT_ELEMENT_SH2(vtx)  vd->nextElement( VFM_SH2, VES_BONEWEIGHT, VET_FLOAT4, KHAOS_MEMBER_OFFSET(vtx, sh8), 3 )
#define KHAOS_NEXT_ELEMENT_SH3(vtx)  vd->nextElement( VFM_SH3, VES_BONEWEIGHT, VET_FLOAT4, KHAOS_MEMBER_OFFSET(vtx, sh12), 4 )

#define KHAOS_NEXT_ELEMENT(x, vtx)  KHAOS_NEXT_ELEMENT_##x(vtx)

#define KHAOS_MAKE_VERTEX_DECL_1( fn, vtx, a ) \
    inline VertexDeclaration* fn() \
    { \
        VertexDeclaration* vd = _createVertexDeclaration( vtx::ID ); \
        KHAOS_NEXT_ELEMENT(a, vtx); \
        vd->setStride( sizeof(vtx) ); \
        return vd; \
    }

#define KHAOS_MAKE_VERTEX_DECL_2( fn, vtx, a, b ) \
    inline VertexDeclaration* fn() \
    { \
        VertexDeclaration* vd = _createVertexDeclaration( vtx::ID ); \
        KHAOS_NEXT_ELEMENT(a, vtx); \
        KHAOS_NEXT_ELEMENT(b, vtx); \
        vd->setStride( sizeof(vtx) ); \
        return vd; \
    }

#define KHAOS_MAKE_VERTEX_DECL_3( fn, vtx, a, b, c ) \
    inline VertexDeclaration* fn() \
    { \
        VertexDeclaration* vd = _createVertexDeclaration( vtx::ID ); \
        KHAOS_NEXT_ELEMENT(a, vtx); \
        KHAOS_NEXT_ELEMENT(b, vtx); \
        KHAOS_NEXT_ELEMENT(c, vtx); \
        vd->setStride( sizeof(vtx) ); \
        return vd; \
    }

#define KHAOS_MAKE_VERTEX_DECL_4( fn, vtx, a, b, c, d ) \
    inline VertexDeclaration* fn() \
    { \
        VertexDeclaration* vd = _createVertexDeclaration( vtx::ID ); \
        KHAOS_NEXT_ELEMENT(a, vtx); \
        KHAOS_NEXT_ELEMENT(b, vtx); \
        KHAOS_NEXT_ELEMENT(c, vtx); \
        KHAOS_NEXT_ELEMENT(d, vtx); \
        vd->setStride( sizeof(vtx) ); \
        return vd; \
    }
   
#define KHAOS_MAKE_VERTEX_DECL_5( fn, vtx, a, b, c, d, e ) \
    inline VertexDeclaration* fn() \
    { \
        VertexDeclaration* vd = _createVertexDeclaration( vtx::ID ); \
        KHAOS_NEXT_ELEMENT(a, vtx); \
        KHAOS_NEXT_ELEMENT(b, vtx); \
        KHAOS_NEXT_ELEMENT(c, vtx); \
        KHAOS_NEXT_ELEMENT(d, vtx); \
        KHAOS_NEXT_ELEMENT(e, vtx); \
        vd->setStride( sizeof(vtx) ); \
        return vd; \
    }

#define KHAOS_MAKE_VERTEX_DECL_6( fn, vtx, a, b, c, d, e, f ) \
    inline VertexDeclaration* fn() \
    { \
        VertexDeclaration* vd = _createVertexDeclaration( vtx::ID ); \
        KHAOS_NEXT_ELEMENT(a, vtx); \
        KHAOS_NEXT_ELEMENT(b, vtx); \
        KHAOS_NEXT_ELEMENT(c, vtx); \
        KHAOS_NEXT_ELEMENT(d, vtx); \
        KHAOS_NEXT_ELEMENT(e, vtx); \
        KHAOS_NEXT_ELEMENT(f, vtx); \
        vd->setStride( sizeof(vtx) ); \
        return vd; \
    }

#define KHAOS_MAKE_VERTEX_DECL_7( fn, vtx, a, b, c, d, e, f, g ) \
    inline VertexDeclaration* fn() \
    { \
        VertexDeclaration* vd = _createVertexDeclaration( vtx::ID ); \
        KHAOS_NEXT_ELEMENT(a, vtx); \
        KHAOS_NEXT_ELEMENT(b, vtx); \
        KHAOS_NEXT_ELEMENT(c, vtx); \
        KHAOS_NEXT_ELEMENT(d, vtx); \
        KHAOS_NEXT_ELEMENT(e, vtx); \
        KHAOS_NEXT_ELEMENT(f, vtx); \
        KHAOS_NEXT_ELEMENT(g, vtx); \
        vd->setStride( sizeof(vtx) ); \
        return vd; \
    }

#define KHAOS_MAKE_VERTEX_DECL_8( fn, vtx, a, b, c, d, e, f, g, h ) \
    inline VertexDeclaration* fn() \
    { \
        VertexDeclaration* vd = _createVertexDeclaration( vtx::ID ); \
        KHAOS_NEXT_ELEMENT(a, vtx); \
        KHAOS_NEXT_ELEMENT(b, vtx); \
        KHAOS_NEXT_ELEMENT(c, vtx); \
        KHAOS_NEXT_ELEMENT(d, vtx); \
        KHAOS_NEXT_ELEMENT(e, vtx); \
        KHAOS_NEXT_ELEMENT(f, vtx); \
        KHAOS_NEXT_ELEMENT(g, vtx); \
        KHAOS_NEXT_ELEMENT(h, vtx); \
        vd->setStride( sizeof(vtx) ); \
        return vd; \
    }

    //////////////////////////////////////////////////////////////////////////
    // KHAOS_MAKE_VERTEX_STRUCT_DECL_n
#define KHAOS_MAKE_VERTEX_STRUCT_DECL_1( name, a ) \
        KHAOS_MAKE_VERTEX_STRUCT_1( name, a ) \
        KHAOS_MAKE_VERTEX_DECL_1( create##name##Declaration, name, a )

#define KHAOS_MAKE_VERTEX_STRUCT_DECL_2( name, a, b ) \
        KHAOS_MAKE_VERTEX_STRUCT_2( name, a, b ) \
        KHAOS_MAKE_VERTEX_DECL_2( create##name##Declaration, name, a, b )
 
#define KHAOS_MAKE_VERTEX_STRUCT_DECL_3( name, a, b, c ) \
        KHAOS_MAKE_VERTEX_STRUCT_3( name, a, b, c ) \
        KHAOS_MAKE_VERTEX_DECL_3( create##name##Declaration, name, a, b, c )

#define KHAOS_MAKE_VERTEX_STRUCT_DECL_4( name, a, b, c, d ) \
        KHAOS_MAKE_VERTEX_STRUCT_4( name, a, b, c, d ) \
        KHAOS_MAKE_VERTEX_DECL_4( create##name##Declaration, name, a, b, c, d )

#define KHAOS_MAKE_VERTEX_STRUCT_DECL_5( name, a, b, c, d, e ) \
        KHAOS_MAKE_VERTEX_STRUCT_5( name, a, b, c, d, e ) \
        KHAOS_MAKE_VERTEX_DECL_5( create##name##Declaration, name, a, b, c, d, e )

#define KHAOS_MAKE_VERTEX_STRUCT_DECL_6( name, a, b, c, d, e, f ) \
        KHAOS_MAKE_VERTEX_STRUCT_6( name, a, b, c, d, e, f ) \
        KHAOS_MAKE_VERTEX_DECL_6( create##name##Declaration, name, a, b, c, d, e, f )

#define KHAOS_MAKE_VERTEX_STRUCT_DECL_7( name, a, b, c, d, e, f, g ) \
        KHAOS_MAKE_VERTEX_STRUCT_7( name, a, b, c, d, e, f, g ) \
        KHAOS_MAKE_VERTEX_DECL_7( create##name##Declaration, name, a, b, c, d, e, f, g )

#define KHAOS_MAKE_VERTEX_STRUCT_DECL_8( name, a, b, c, d, e, f, g, h ) \
        KHAOS_MAKE_VERTEX_STRUCT_8( name, a, b, c, d, e, f, g, h ) \
        KHAOS_MAKE_VERTEX_DECL_8( create##name##Declaration, name, a, b, c, d, e, f, g, h )
}

