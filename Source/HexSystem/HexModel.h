#pragma once

#include "CoreMinimal.h"
#include "Containers/Set.h"

class FHexModel
{
public:
    template<typename T>
    struct THex
    {
        // Cube storage, cube constructor
        THex( T q_, T r_, T s_ ) : q( q_ ), r( r_ ), s( s_ )
        {
            check( abs(q + r + s) <= FLT_EPSILON );
        }

        // Cube storage, axial constructor
        THex( T q_, T r_ ) : q( q_ ), r( r_ ), s( -q_ - r_ )
        {
            check( abs( q + r + s ) <= FLT_EPSILON );
        }

        const T q, r, s;
    };

    typedef THex<int> FHex;
    typedef THex<double> FFractionalHex;

    static FHex HexAdd( FHex a, FHex b )
    {
        return FHex( a.q + b.q, a.r + b.r, a.s + b.s );
    }

    static FHex HexSubtract( FHex a, FHex b )
    {
        return FHex( a.q - b.q, a.r - b.r, a.s - b.s );
    }

    static FHex HexMultiply( FHex a, int k )
    {
        return FHex( a.q * k, a.r * k, a.s * k );
    }

    static int HexLength( FHex hex )
    {
        return int( ( abs( hex.q ) + abs( hex.r ) + abs( hex.s ) ) / 2 );
    }

    static int HexDistance( FHex a, FHex b )
    {
        return HexLength( HexSubtract( a, b ) );
    }

    static const TArray<FHex> HexDirections;

    // Top-Right Clock-Wise Direction
    static FHex HexDirection( int direction /* 0 to 5 */ )
    {
        check( 0 <= direction && direction < 6 );
        return HexDirections[ direction ];
    }

    static FHex HexNeighbor( FHex hex, int direction )
    {
        return HexAdd( hex, HexDirection( direction ) );
    }

    //////////////////////////////////////////////////////////////////////////////////////

    struct FOrientation
    {
        const double f0, f1, f2, f3;
        const double b0, b1, b2, b3;
        const double start_angle; // in multiples of 60°
        FOrientation( double f0_, double f1_, double f2_, double f3_,
                      double b0_, double b1_, double b2_, double b3_,
                      double start_angle_ )
            : f0( f0_ ), f1( f1_ ), f2( f2_ ), f3( f3_ ),
            b0( b0_ ), b1( b1_ ), b2( b2_ ), b3( b3_ ),
            start_angle( start_angle_ )
        {
        }
    };

    static const FOrientation LayoutPointy;

    static const FOrientation LayoutFlat;

    using FPoint = FVector2D;
    using FSize = FVector2D;
    using FOrigin = FVector2D;

    struct FLayout
    {
        const FOrientation orientation;
        const FSize size;
        const FOrigin origin;
        FLayout( const FOrientation orientation_, const FSize size_, const FOrigin origin_ )
            : orientation( orientation_ ), size( size_ ), origin( origin_ )
        {
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////

    static FPoint HexToPixel( FLayout layout, FHex h )
    {
        const FOrientation& M = layout.orientation;
        double x = ( M.f0 * h.q + M.f1 * h.r ) * layout.size.X;
        double y = ( M.f2 * h.q + M.f3 * h.r ) * layout.size.Y;
        return FPoint( x + layout.origin.X, y + layout.origin.Y );
    }

    static FFractionalHex PixelToHex( FLayout layout, FPoint p )
    {
        const FOrientation& M = layout.orientation;
        FPoint pt = FPoint( ( p.X - layout.origin.X ) / layout.size.X, ( p.Y - layout.origin.Y ) / layout.size.Y );
        double q = M.b0 * pt.X + M.b1 * pt.Y;
        double r = M.b2 * pt.X + M.b3 * pt.Y;
        return FFractionalHex( q, r, -q - r );
    }

    static FPoint HexCornerOffset( FLayout layout, int corner )
    {
        FPoint size = layout.size;
        double angle = 2.0 * PI * ( layout.orientation.start_angle + corner ) / 6;
        return FPoint( size.X * cos( angle ), size.Y * sin( angle ) );
    }

    static TArray<FPoint> PolygonCorners( FLayout layout, FHex h )
    {
        TArray<FPoint> corners = {};
        FPoint center = HexToPixel( layout, h );
        for ( int i = 0; i < 6; i++ )
        {
            FPoint offset = HexCornerOffset( layout, i );
            corners.Push( FPoint( center.X + offset.X, center.Y + offset.Y ) );
        }
        return corners;
    }

    static FHex HexRound( FFractionalHex h )
    {
        int q = int( round( h.q ) );
        int r = int( round( h.r ) );
        int s = int( round( h.s ) );
        double q_diff = abs( q - h.q );
        double r_diff = abs( r - h.r );
        double s_diff = abs( s - h.s );
        if ( q_diff > r_diff && q_diff > s_diff )
        {
            q = -r - s;
        }
        else if ( r_diff > s_diff )
        {
            r = -q - s;
        }
        else
        {
            s = -q - r;
        }
        return FHex( q, r, s );
    }

    static void GenerateHexMap( TSet<FHex>& outHexMap, int mapRadius)
    {
        for ( int q = -mapRadius; q <= mapRadius; q++ )
        {
            int r1 = FMath::Max( -mapRadius, -q - mapRadius );
            int r2 = FMath::Min( mapRadius, -q + mapRadius );
            for ( int r = r1; r <= r2; r++ )
            {
                outHexMap.Add( FHex( q, r, -q - r ) );
            }
        }
    }
};

bool operator == ( const FHexModel::FHex& a, const FHexModel::FHex& b );
bool operator != ( const FHexModel::FHex& a, const FHexModel::FHex& b );
bool operator == ( const FHexModel::FFractionalHex& a, const FHexModel::FFractionalHex& b );
bool operator != ( const FHexModel::FFractionalHex& a, const FHexModel::FFractionalHex& b );

#if UE_BUILD_DEBUG
uint32 GetTypeHash( const FHexModel::FHex& hex );
#else // optimize by inlining in shipping and development builds
FORCEINLINE uint32 GetTypeHash( const FHexModel::FHex& hex )
{
    uint32 hashQ = GetTypeHash( hex.q );
    uint32 hashR = GetTypeHash( hex.r );
    uint32 result = HashCombine( hashQ, hashR );

    return result;
}
#endif