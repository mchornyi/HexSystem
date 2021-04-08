#include "Misc/AutomationTest.h"
#include "../HexModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FHexModelTest, "HexModelTests", EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter );

namespace
{
    FString ToString( const FHexModel::FHex& hex )
    {
        return FString::Printf( TEXT( "hex:[%d,%d,%d]" ), hex.q, hex.r, hex.s );
    }

    struct AutomationTestProxy
    {
        bool TestEqual( const TCHAR* What, const FHexModel::FHex& Actual, const FHexModel::FHex& Expected ) const
        {
            if ( Expected != Actual )
            {
                AutomationTestBase->AddError( FString::Printf( TEXT( "Expected '%s' to be %s, but it was %s." ), What, *ToString( Expected ), *ToString( Actual ) ), 1 );
                return false;
            }
            return true;
        }

        FAutomationTestBase* AutomationTestBase;
    };
}

bool FHexModelTest::RunTest( const FString& Parameters )
{
    const AutomationTestProxy proxy{this};

    {
        FHexModel::FHex a( 1, -1, 0 );
        FHexModel::FHex b( -3, 2, 1 );

        FHexModel::FHex result = FHexModel::HexAdd( a, b );

        proxy.TestEqual( TEXT( "Add two hexes" ), result, FHexModel::FHex( -2, 1, 1 ) );
    }

    {
        FHexModel::FHex a( 1, -1, 0 );
        FHexModel::FHex b( -3, 2, 1 );

        FHexModel::FHex result = FHexModel::HexSubtract( a, b );

        proxy.TestEqual( TEXT( "Subtract two hexes" ), result, FHexModel::FHex( 4, -3, -1 ) );
    }

    return true;
}