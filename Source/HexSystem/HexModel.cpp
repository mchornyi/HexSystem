#include "HexModel.h"

using namespace hexsystem;

const TArray<FHex> FHexModel::HexDirections = {
        FHex( 1, -1, 0 ), FHex( 1, 0, -1 ), FHex( 0, 1, -1 ),
        FHex( -1, 1, 0 ), FHex( -1, 0, 1 ), FHex( 0, -1, 1 )
};

bool operator == ( const FHex& a, const FHex& b )
{
    return a.q == b.q && a.r == b.r && a.s == b.s;
}

bool operator != ( const FHex& a, const FHex& b )
{
    return !( a == b );
}

bool operator == ( const FFractionalHex& a, const FFractionalHex& b )
{
    return a.q == b.q && a.r == b.r && a.s == b.s;
}

bool operator != ( const FFractionalHex& a, const FFractionalHex& b )
{
    return !( a == b );
}

#if UE_BUILD_DEBUG
uint32 GetTypeHash( const FHex& hex )
{
    uint32 hashQ = GetTypeHash( hex.q );
    uint32 hashR = GetTypeHash( hex.r );
    uint32 result = HashCombine( hashQ, hashR );

    return result;
}
#endif