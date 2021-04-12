#include "HexModel.h"

const FHexModel::FOrientation FHexModel::LayoutPointy
= FHexModel::FOrientation( sqrt( 3.0f ), sqrt( 3.0f ) / 2.0f, 0.0f, 3.0f / 2.0f, sqrt( 3.0f ) / 3.0f, -1.0f / 3.0f, 0.0f, 2.0f / 3.0f, 0.5f );

const FHexModel::FOrientation FHexModel::LayoutFlat
= FHexModel::FOrientation( 3.0f / 2.0f, 0.0f, sqrt( 3.0f ) / 2.0f, sqrt( 3.0f ), 2.0f / 3.0f, 0.0f, -1.0f / 3.0f, sqrt( 3.0f ) / 3.0f, 0.0f);

const TArray<FHexModel::FHex> FHexModel::HexDirections = {
        FHex( 1, -1, 0 ), FHex( 1, 0, -1 ), FHex( 0, 1, -1 ),
        FHex( -1, 1, 0 ), FHex( -1, 0, 1 ), FHex( 0, -1, 1 )
};

bool operator == ( const FHexModel::FHex& a, const FHexModel::FHex& b )
{
    return a.q == b.q && a.r == b.r && a.s == b.s;
}

bool operator != ( const FHexModel::FHex& a, const FHexModel::FHex& b )
{
    return !( a == b );
}

bool operator == ( const FHexModel::FFractionalHex& a, const FHexModel::FFractionalHex& b )
{
    return a.q == b.q && a.r == b.r && a.s == b.s;
}

bool operator != ( const FHexModel::FFractionalHex& a, const FHexModel::FFractionalHex& b )
{
    return !( a == b );
}

#if UE_BUILD_DEBUG
uint32 GetTypeHash( const FHexModel::FHex& hex )
{
    uint32 hashQ = GetTypeHash( hex.q );
    uint32 hashR = GetTypeHash( hex.r );
    uint32 result = HashCombine( hashQ, hashR );

    return result;
}
#endif