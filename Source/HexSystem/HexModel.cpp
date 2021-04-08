#include "HexModel.h"

const FHexModel::FOrientation FHexModel::LayoutPointy
= FHexModel::FOrientation( sqrt( 3.0 ), sqrt( 3.0 ) / 2.0, 0.0, 3.0 / 2.0, sqrt( 3.0 ) / 3.0, -1.0 / 3.0, 0.0, 2.0 / 3.0, 0.5 );

const FHexModel::FOrientation FHexModel::LayoutFlat
= FHexModel::FOrientation( 3.0 / 2.0, 0.0, sqrt( 3.0 ) / 2.0, sqrt( 3.0 ), 2.0 / 3.0, 0.0, -1.0 / 3.0, sqrt( 3.0 ) / 3.0, 0.0 );

const TArray<FHexModel::FHex> FHexModel::HexDirections = {
        FHex( 1, 0, -1 ), FHex( 1, -1, 0 ), FHex( 0, -1, 1 ),
        FHex( -1, 0, 1 ), FHex( -1, 1, 0 ), FHex( 0, 1, -1 )
};

bool operator == ( const FHexModel::FHex& a, const FHexModel::FHex& b )
{
    return a.q == b.q && a.r == b.r && a.s == b.s;
}

bool operator != ( const FHexModel::FHex& a, const FHexModel::FHex& b )
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