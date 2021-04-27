#include "Misc/AutomationTest.h"
#include "../Online/HexCellNode.h"
#include "../Online/HexSpatialization2DNode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FHexReplicationGraphTest, "HexReplicationGraphTests", EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter );

using namespace hexsystem;

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

    const FHexLayout sHexLayout( LayoutPointy, FSize( 100, 100 ), FOrigin( 0.0f, 0.0f ) );
}

bool FHexReplicationGraphTest::RunTest( const FString& Parameters )
{
    //Teset: HexCoverage
    {
        UReplicationGraphNode_HexSpatialization2D* hexSpatialization2D = NewObject<UReplicationGraphNode_HexSpatialization2D>();

        const TArray<UReplicationGraphNode_HexCell*> actual = hexSpatialization2D->GetHexCellCoverage( { -306.109924f, -84.516525f, 0.0f}, 200.0f );

        const FHex hexArr[ ] = { {-1, 0, 1}, {-1, -1, 2}, {0, -1, 1}, {0, 0, 0}, {-1, 1, 0} };
        TArray<FHex> expectedHexes( hexArr, UE_ARRAY_COUNT( hexArr ) );

        for (const FHex& expected: expectedHexes )
        {
            const auto result = actual.FindByPredicate( [&]( const UReplicationGraphNode_HexCell* hexCell){
                return hexCell->GetHex( ) == expected;
            } );

            TestTrue( TEXT( "HexCoverage" ), result != nullptr );
        }

        //TODO: How to delete hexSpatialization2D
    }

    return true;
}