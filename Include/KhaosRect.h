#pragma once
#include "KhaosStdTypes.h"

namespace Khaos
{
    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    struct TIntVector2
    {
        TIntVector2() : x(0), y(0) {}
        TIntVector2( T x1, T y1 ) : x(x1), y(y1) {}

        const T& operator[]( size_t i ) const
        {
            khaosAssert( i < 2 );
            return *(&x + i);
        }

        T& operator[]( size_t i )
        {
            khaosAssert( i < 2 );
            return *(&x + i);
        }

        T x;
        T y;
    };

    typedef TIntVector2<int>  IntVector2;
    typedef TIntVector2<uint> UIntVector2;


    //////////////////////////////////////////////////////////////////////////
    struct IntRect
    {
    public:
        IntRect() : left(0), top(0), right(0), bottom(0) {}
        IntRect( int l, int t, int r, int b ) : left(l), top(t), right(r), bottom(b) {}

    public:
        void set( int l, int t, int r, int b )
        {
            left   = l;
            top    = t;
            right  = r;
            bottom = b;
        }

        void make( int l, int t, int w, int h )
        {
            left   = l;
            top    = t;
            right  = l + w;
            bottom = t + h;
        }

		void setEmpty()
		{
			left   = INT_MAX;
			top    = INT_MAX;
			right  = INT_MIN;
			bottom = INT_MIN;
		}

		void merge( const IntRect& rhs )
		{
			if ( rhs.left < left )
				left = rhs.left;

			if ( rhs.top < top )
				top = rhs.top;

			if ( rhs.right > right )
				right = rhs.right;

			if ( rhs.bottom > bottom )
				bottom = rhs.bottom;
		}

		void intersect( const IntRect& rhs )
		{
			if ( rhs.left > left )
				left = rhs.left;

			if ( rhs.top > top )
				top = rhs.top;

			if ( rhs.right < right )
				right = rhs.right;

			if ( rhs.bottom < bottom )
				bottom = rhs.bottom;
		}

        int getWidth()  const { return right - left; }
        int getHeight() const { return bottom - top; }

    public:
        int left;
        int top;
        int right;
        int bottom;
    };

    //////////////////////////////////////////////////////////////////////////
    struct IntBox
    {
    public:
        IntBox() : left(0), top(0), right(0), bottom(0), front(0), back(0) {}
        IntBox( int l, int t, int r, int b, int fr, int bk ) : left(l), top(t), right(r), bottom(b), front(fr), back(bk) {}

        void set( int l, int t, int r, int b, int fr, int bk )
        {
            left   = l;
            top    = t;
            right  = r;
            bottom = b;
            front  = fr;
            back   = bk;
        }

        void make( int l, int t, int w, int h, int fr, int dp )
        {
            left   = l;
            top    = t;
            right  = l + w;
            bottom = t + h;
            front  = fr;
            back   = fr + dp;
        }

        int getWidth()  const { return right - left; }
        int getHeight() const { return bottom - top; }
        int getDepth()  const { return back - front; }

    public:
        int left;
        int top;
        int right;
        int bottom;
        int front;
        int back;
    };
}

