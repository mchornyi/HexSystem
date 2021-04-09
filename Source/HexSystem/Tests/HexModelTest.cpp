#include "Misc/AutomationTest.h"
#include "../HexModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FHexModelTest, "HexModelTests", EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter );

namespace
{
    template<typename T>
    FString ToString( const T& hex )
    {
        return FString::Printf( TEXT( "hex:[%d,%d,%d]" ), hex.q, hex.r, hex.s );
    }

    struct AutomationTestProxy
    {
        template<typename T>
        bool TestEqual( const TCHAR* What, const T& Actual, const T& Expected ) const
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
        const FHexModel::FHex a( 1, -1, 0 );
        const FHexModel::FHex b( -3, 2, 1 );
        const FHexModel::FHex result = FHexModel::HexAdd( a, b );
        proxy.TestEqual( TEXT( "HexAdd" ), result, { -2, 1, 1 } );
    }

    {
        const FHexModel::FHex a( 1, -1, 0 );
        const FHexModel::FHex b( -3, 2, 1 );
        const FHexModel::FHex result = FHexModel::HexSubtract( a, b );
        proxy.TestEqual( TEXT( "HexSubtract" ), result, { 4, -3, -1 } );
    }

    {
        const FHexModel::FHex a( 1, -1, 0 );
        const FHexModel::FHex result = FHexModel::HexMultiply( a, 2 );
        proxy.TestEqual( TEXT( "HexMultiply" ), result, { 2, -2, 0 } );
    }

    {
        const FHexModel::FHex a( -3, 4, -1 );
        const int result = FHexModel::HexLength( a );
        TestEqual( TEXT( "HexLength" ), result, 4 );
    }

    {
        const FHexModel::FHex a( -3, 4, -1 );
        const FHexModel::FHex b( 4, -3, -1 );
        const int result = FHexModel::HexDistance( a, b );
        TestEqual( TEXT( "HexDistance" ), result, 7 );
    }

    {
        const FHexModel::FHex result = FHexModel::HexDirection( 3 );
        proxy.TestEqual( TEXT( "HexDirection" ), result, { -1, 1, 0 } );
    }

    {
        const FHexModel::FHex a( 2, -3, 1 );
        const FHexModel::FHex result = FHexModel::HexNeighbor( a, 4 );
        proxy.TestEqual( TEXT( "HexNeighbor" ), result, { 1, -3, 2 } );
    }

    {
        TSet<FHexModel::FHex> hexMapActual;
        TSet<FHexModel::FHex> hexMapExpected;
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 0, 0, 0 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 1, -1, 0 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 1, 0, -1 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 0, 1, -1 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { -1, 1, 0 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { -1, 0, 1 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 0, -1, 1 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 1, -2, 1 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 2, -2, 0 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 2, -1, -1 } ).IsValidId( ), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 2, 0, -2 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 1, 1, -2 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 0, 2, -2 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { -1, 2, -1 } ).IsValidId( ), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { -2, 2, 0 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { -2, 1, 1 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { -2, 0, 2 } ).IsValidId(), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { -1, -1, 2 } ).IsValidId( ), true );
        TestEqual( TEXT( "GenerateHexMap: Add to container" ), hexMapExpected.Add( { 0, -2, 2 } ).IsValidId( ), true );

        FHexModel::GenerateHexMap( hexMapActual, 2 );

        TestEqual( TEXT( "GenerateHexMap: Test Size" ), hexMapActual.Num(), hexMapExpected.Num( ) );

        for ( const auto& hex : hexMapActual )
        {
            TestEqual( TEXT( "GenerateHexMap: Test Content" ), hexMapExpected.Contains( hex ), true );
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Test Layout
    const FHexModel::FLayout hexLayout( FHexModel::LayoutPointy, FHexModel::FSize( 100, 100 ), FHexModel::FOrigin( -1050, -40 ) );
    {
        const FHexModel::FHex a( 0, 3, -3 );
        FHexModel::FPoint result = FHexModel::HexToPixel( hexLayout, a );
        TestEqual( TEXT( "HexToPixel" ), result, {-790.192383f, 410.0f } );
    }

    {
        FHexModel::FPoint result = FHexModel::HexCornerOffset( hexLayout, 3 );
        TestEqual( TEXT( "HexCornerOffset" ), result, { -86.6025391f, -50.0000076f } );
    }

    {
        const FHexModel::FFractionalHex fHex = FHexModel::PixelToHex( hexLayout, { -1047.03284f, 288.912903f } );
        const FHexModel::FHex hex = FHexModel::HexRound( fHex );
        proxy.TestEqual( TEXT( "PixelToHex/HexRound" ), hex, { -1, 2, -1 } );
    }

    {
        TArray<FHexModel::FPoint> result = FHexModel::PolygonCorners( hexLayout, { 4, -4, 0 } );
        TestEqual( TEXT( "PolygonCorners" ), result, { { -616.987305f, -590.0 }, { -703.589844f, -540.0f }, { -790.192383f, -590.0f }, { -790.192383f, -690.0f }, { -703.589844f, -740.0f }, { -616.987305f, -690.0f } } );
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////

    return true;
}