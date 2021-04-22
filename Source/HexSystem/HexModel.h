#pragma once

#include "CoreMinimal.h"
#include "Containers/Set.h"
#include "GameFramework/Actor.h"
#include "HexModel.generated.h"

namespace hexsystem
{
    struct FFractionalHex
    {
        // Cube storage, cube constructor
        FFractionalHex( float q_, float r_, float s_ ) : q( q_ ), r( r_ ), s( s_ )
        {
            check( abs( q + r + s ) <= FLT_EPSILON );
        }

        // Cube storage, axial constructor
        FFractionalHex( float q_, float r_ ) : q( q_ ), r( r_ ), s( -q_ - r_ )
        {
            check( abs( q + r + s ) <= FLT_EPSILON );
        }

        float q, r, s;
    };

    using FPoint = FVector2D;
    using FSize = FVector2D;
    using FOrigin = FVector2D;

    struct FOrientation
    {
        float f0, f1, f2, f3;
        float b0, b1, b2, b3;
        float start_angle; // in multiples of 60°
        FOrientation( float f0_, float f1_, float f2_, float f3_,
                      float b0_, float b1_, float b2_, float b3_,
                      float start_angle_ )
            : f0( f0_ ), f1( f1_ ), f2( f2_ ), f3( f3_ ), b0( b0_ ), b1( b1_ ), b2( b2_ ), b3( b3_ ), start_angle( start_angle_ )
        {
        }
    };

    const FOrientation LayoutPointy = FOrientation( sqrt( 3.0f ), sqrt( 3.0f ) / 2.0f, 0.0f, 3.0f / 2.0f, sqrt( 3.0f ) / 3.0f, -1.0f / 3.0f, 0.0f, 2.0f / 3.0f, 0.5f );

    const FOrientation LayoutFlat = FOrientation( 3.0f / 2.0f, 0.0f, sqrt( 3.0f ) / 2.0f, sqrt( 3.0f ), 2.0f / 3.0f, 0.0f, -1.0f / 3.0f, sqrt( 3.0f ) / 3.0f, 0.0f );
}

USTRUCT( )
struct FHexLevelInfo
{
    GENERATED_USTRUCT_BODY( )

        FHexLevelInfo( )
    {
    }

    UPROPERTY( EditInstanceOnly, Category = "HexLevelInfo" )
        FName LevelName;
};

USTRUCT( )
struct FHex
{
    GENERATED_USTRUCT_BODY( )

        FHex( ) : q( 0 ), r( 0 ), s( 0 )
    {
    }

    // Cube storage, cube constructor
    FHex( int16 q_, int16 r_, int16 s_ ) : q( q_ ), r( r_ ), s( s_ )
    {
        check( abs( q + r + s ) <= FLT_EPSILON );
    }

    // Cube storage, axial constructor
    FHex( int16 q_, int16 r_ ) : q( q_ ), r( r_ ), s( -q_ - r_ )
    {
        check( abs( q + r + s ) <= FLT_EPSILON );
    }

    bool IsZero( ) const
    {
        return q == 0 && r == 0 && s == 0;
    };

    UPROPERTY( )
        int16 q;

    UPROPERTY( )
        int16 r;

    UPROPERTY( )
        int16 s;

    UPROPERTY( EditInstanceOnly, Category = "HexLevelInfo" )
        FHexLevelInfo LevelInfo;
};

USTRUCT( )
struct FHexLayout
{
    GENERATED_USTRUCT_BODY( )

        hexsystem::FOrientation orientation;

    UPROPERTY( EditInstanceOnly, Category = "HexWorldParams", Meta = ( ClampMin = "1", ClampMax = "100000" ) )
        FVector2D size;

    FVector2D origin;

    FHexLayout( const hexsystem::FOrientation orientation_, const hexsystem::FSize size_, const hexsystem::FOrigin origin_ )
        : orientation( orientation_ ), size( size_ ), origin( origin_ )
    {
    }

    FHexLayout( ) : orientation( hexsystem::LayoutPointy ), size( 100, 100 ), origin( 0.0f, 0.0f )
    {
    }
};

bool operator == ( const struct FHex& a, const struct FHex& b );
bool operator != ( const struct FHex& a, const struct FHex& b );
bool operator == ( const hexsystem::FFractionalHex& a, const hexsystem::FFractionalHex& b );
bool operator != ( const hexsystem::FFractionalHex& a, const hexsystem::FFractionalHex& b );

namespace hexsystem
{
    class FHexModel
    {
    public:

        static FHex HexAdd( const FHex a, const FHex b )
        {
            return FHex( a.q + b.q, a.r + b.r, a.s + b.s );
        }

        static FHex HexSubtract( const FHex a, const FHex b )
        {
            return FHex( a.q - b.q, a.r - b.r, a.s - b.s );
        }

        static FHex HexMultiply( const FHex a, const int k )
        {
            return FHex( a.q * k, a.r * k, a.s * k );
        }

        static int HexLength( const FHex hex )
        {
            return int( ( abs( hex.q ) + abs( hex.r ) + abs( hex.s ) ) / 2 );
        }

        static int HexDistance( const FHex a, const FHex b )
        {
            return HexLength( HexSubtract( a, b ) );
        }

        // Top-Right Clock-Wise Direction
        static FHex HexDirection( const uint16 direction /* 0 to 5 */ )
        {
            check( 0 <= direction && direction < 6 );
            return HexDirections[ direction ];
        }

        static FHex HexNeighbor( const FHex hex, const int direction )
        {
            return HexAdd( hex, HexDirection( direction ) );
        }

        static FHex HexScale( const FHex a, const uint16 ring )
        {
            return { a.q * ring, a.r * ring, a.s * ring };
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        static FPoint HexToPixel( const FHexLayout layout, const FHex h )
        {
            const FOrientation& M = layout.orientation;
            float x = ( M.f0 * h.q + M.f1 * h.r ) * layout.size.X;
            float y = ( M.f2 * h.q + M.f3 * h.r ) * layout.size.Y;
            return FPoint( x + layout.origin.X, y + layout.origin.Y );
        }

        static FFractionalHex PixelToHex( const FHexLayout layout, const FPoint p )
        {
            const FOrientation& M = layout.orientation;
            FPoint pt = FPoint( ( p.X - layout.origin.X ) / layout.size.X, ( p.Y - layout.origin.Y ) / layout.size.Y );
            float q = M.b0 * pt.X + M.b1 * pt.Y;
            float r = M.b2 * pt.X + M.b3 * pt.Y;
            return FFractionalHex( q, r, -q - r );
        }

        static FPoint HexCornerOffset( const FHexLayout layout, const int corner )
        {
            FPoint size = layout.size;
            float angle = 2.0 * PI * ( layout.orientation.start_angle + corner ) / 6;
            return FPoint( size.X * cos( angle ), size.Y * sin( angle ) );
        }

        static TArray<FPoint> PolygonCorners( const FHexLayout layout, const FHex h )
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

        static FHex HexRound( const FFractionalHex h )
        {
            int q = int( round( h.q ) );
            int r = int( round( h.r ) );
            int s = int( round( h.s ) );
            float q_diff = abs( q - h.q );
            float r_diff = abs( r - h.r );
            float s_diff = abs( s - h.s );

            if ( q_diff > r_diff && q_diff > s_diff )
                q = -r - s;
            else if ( r_diff > s_diff )
                r = -q - s;
            else
                s = -q - r;

            return FHex( q, r, s );
        }

        // Generates hexagonal shape
        static void GenerateHexMap( TSet<FHex>& outHexMap, int mapRadius )
        {
            outHexMap.Reset( );
            for ( int q = -mapRadius; q <= mapRadius; q++ )
            {
                const int r1 = FMath::Max( -mapRadius, -q - mapRadius );
                const int r2 = FMath::Min( mapRadius, -q + mapRadius );
                for ( int r = r1; r <= r2; r++ )
                {
                    outHexMap.Add( FHex( q, r, -q - r ) );
                }
            }
        }

        static TArray<FHex> HexRing( const FHex hexCenter, const uint32 ring )
        {
            TArray<FHex> result;
            result.AddUnique( hexCenter );

            if ( ring == 0 )
                return result;

            FHex hex = HexAdd( hexCenter, HexScale( HexDirection( 4 ), ring ) );

            for ( uint16 i = 0; i < 6; ++i )
            {
                for ( uint16 r = 0; r < ring; ++r )
                {
                    result.AddUnique( hex );
                    hex = HexNeighbor( hex, i );
                }
            }

            return result;
        }

        static TArray<FHex> HexCoverage( const FHexLayout layout, const FVector2D originLoc, float cullDist, uint16 maxDist, uint16 maxRing = 100 )
        {
            check( cullDist > 0 );

            maxRing = FMath::Clamp( maxRing, ( uint16 )0, ( uint16 )100 );

            TArray<FHex> result;

            const FHex hexOrigin = HexRound( PixelToHex( layout, originLoc ) );
            const uint16 distToHexZero = HexDistance( {}, hexOrigin );
            if ( distToHexZero <= maxDist )
                result.AddUnique( hexOrigin );

            bool mustStopSearching = false; // If none of the hex gets covered in the ring
            for ( uint16 i = 1; i <= maxRing && !mustStopSearching; ++i )
            {
                mustStopSearching = true;

                TArray<FHex> hexOfRing = HexRing( hexOrigin, i );
                for ( const FHex& hex : hexOfRing )
                {
                    const uint16 hexDistToHexZero = HexDistance( {}, hex );

                    const FVector2D hexLocation = HexToPixel( layout, hex );
                    const float distToOriginSq = ( hexLocation - originLoc ).SizeSquared( );
                    if ( distToOriginSq <= cullDist * cullDist && hexDistToHexZero <= maxDist )
                    {
                        result.AddUnique( hex );
                        mustStopSearching = false;
                        continue;
                    }

                    const FVector2D dir = ( hexLocation - originLoc ).GetSafeNormal( );
                    const FVector2D hexCoveredLoc = originLoc + dir * cullDist;

                    const FHex hexCovered = HexRound( PixelToHex( layout, hexCoveredLoc ) );
                    if ( hexCovered == hex && hexDistToHexZero <= maxDist )
                    {
                        result.AddUnique( hex );
                        mustStopSearching = false;
                    }
                }
            }

            return result;
        }

    public:
        static const TArray<FHex> HexDirections;
    };
};

#if UE_BUILD_DEBUG
uint32 GetTypeHash( const FHex& hex );
#else // optimize by inlining in shipping and development builds
FORCEINLINE uint32 GetTypeHash( const FHex& hex )
{
    uint32 hashQ = GetTypeHash( hex.q );
    uint32 hashR = GetTypeHash( hex.r );
    uint32 result = HashCombine( hashQ, hashR );

    return result;
}
#endif
