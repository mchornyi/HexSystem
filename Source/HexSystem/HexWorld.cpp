// Fill out your copyright notice in the Description page of Project Settings.


#include "HexWorld.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

DECLARE_LOG_CATEGORY_EXTERN( HexTrace, Log, All );
DEFINE_LOG_CATEGORY( HexTrace );

using namespace hexsystem;

namespace
{
	void DrawDebugHex( UWorld* inWorld, const FHexLayout& layout, const FHex& hex, float posZ, bool isActorInside, bool isHexVisible );
	void DrawCharacterPos( UWorld* inWorld );

	static TAutoConsoleVariable<int> CVar_HexRingsNum( TEXT( "hex.HexRingsNum" ), 6, TEXT( "Defines the number of hex rings." ), ECVF_Default );
	static TAutoConsoleVariable<int> CVar_HexDistVisibility( TEXT( "hex.HexDistVisibility" ), 3, TEXT( "Defines the distance of hex visibility in a context of hex model." ), ECVF_Default );
    static TAutoConsoleVariable<float> CVar_HexSize( TEXT( "hex.HexSize" ), 100.0f, TEXT( "Defines the size of hex cell." ), ECVF_Default);
	static TAutoConsoleVariable<float> CVar_LineThickness( TEXT( "hex.LineThickness" ), 10.0f, TEXT( "Defines the hex line thickness for debug visualizing." ), ECVF_Default );
}

// Sets default values
AHexWorld::AHexWorld() :
	HexLayout(LayoutPointy, FSize( 100, 100 ), FOrigin( 0, 0 ) )
	, HexRingsNum(6)
	, HexDistVisibility( 3 )
	, bHexGenerateTrigger(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    CVar_HexSize.AsVariable( )->SetOnChangedCallback( FConsoleVariableDelegate::CreateLambda( [ this ]( IConsoleVariable* Variable )
	{
		UE_LOG( HexTrace, Display, TEXT( __FUNCTION__ ) );
		HexLayout.size = { CVar_HexSize.GetValueOnGameThread( ) , CVar_HexSize.GetValueOnGameThread( ) };
	} ) );

	CVar_HexRingsNum.AsVariable( )->SetOnChangedCallback( FConsoleVariableDelegate::CreateLambda( [ this ]( IConsoleVariable* Variable )
	{
		UE_LOG( HexTrace, Display, TEXT( __FUNCTION__ ) );
		HexRingsNum = CVar_HexRingsNum.GetValueOnGameThread( );
		Generate( );
	} ) );

	CVar_HexDistVisibility.AsVariable( )->SetOnChangedCallback( FConsoleVariableDelegate::CreateLambda( [ this ]( IConsoleVariable* Variable )
    {
		UE_LOG( HexTrace, Display, TEXT( __FUNCTION__ ) );
        HexDistVisibility = CVar_HexDistVisibility.GetValueOnGameThread( );
    } ) );
}

// Called when the game starts or when spawned
void AHexWorld::BeginPlay()
{
	Super::BeginPlay();
}

void AHexWorld::PostEditChangeProperty( FPropertyChangedEvent& e )
{
	UE_LOG( HexTrace, Display, TEXT( __FUNCTION__ ) );

    FName propertyName = ( e.Property != NULL ) ? e.Property->GetFName( ) : NAME_None;
    if ( propertyName == GET_MEMBER_NAME_CHECKED( AHexWorld, bHexGenerateTrigger ) )
    {
		if ( bHexGenerateTrigger == true )
		{
			Generate( );
			bHexGenerateTrigger = false;
		}
    }

    Super::PostEditChangeProperty( e );
}

////for TArrays:
//void ACustomClass::PostEditChangeChainProperty( struct FPropertyChangedChainEvent& e )
//{
//    int32 index = e.GetArrayIndex( TEXT( "Meshes" ) ); //checks skipped
//    UStaticMesh* mesh = Meshes[ index ]; //changed mesh
//    Super::PostEditChangeChainProperty( e );
//}

void AHexWorld::Generate( )
{
	UE_LOG( HexTrace, Display, TEXT( __FUNCTION__ ) );

	if ( HexRingsNum == HexMap.Num( ) )
	{
		return;
	}

	FHexModel::GenerateHexMap( HexMap, HexRingsNum );
}

// Called every frame
void AHexWorld::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector& actorLoc = GetTransform( ).GetLocation( );
	HexLayout.origin = { actorLoc.X, actorLoc.Y };

	FHex currentHex{ 0, 0, 0 };
	if ( ACharacter* tmpCharacter = UGameplayStatics::GetPlayerCharacter( GetWorld( ), 0 ) )
	{
        const FFractionalHex fHex = FHexModel::PixelToHex( HexLayout, { tmpCharacter->GetActorLocation( ).X, tmpCharacter->GetActorLocation( ).Y } );
        currentHex = FHexModel::HexRound( fHex );
	}

	//const bool isActorInsideHexMap = HexMap.Contains( currentHex );

	for ( const auto& hex : HexMap )
	{
        const bool isActorInside = (hex == currentHex);
		const bool isHexVisible = FHexModel::HexDistance( hex, currentHex ) <= HexDistVisibility;

        DrawDebugHex( GetWorld( ), HexLayout, hex, actorLoc.Z, isActorInside, isHexVisible );
	}

	DrawCharacterPos( GetWorld( ) );
}

#ifdef WITH_EDITOR
bool AHexWorld::ShouldTickIfViewportsOnly( ) const
{
    return true;
}
#endif


//void AHexWorld::UpdateHexParams( )
//{
//	UE_LOG( LogTrace, Display, TEXT( __FUNCTION__ ) );
//    HexLayout.size = { CVar_HexSize.GetValueOnGameThread( ) , CVar_HexSize.GetValueOnGameThread( ) };
//}

//void AHexWorld::OnChangeCVar( IConsoleVariable* var )
//{
//
//}

namespace
{
	void DrawDebugHex( UWorld* inWorld, const FHexLayout& layout, const FHex& hex, float posZ, bool isActorInside, bool isHexVisible )
	{
		check( inWorld );

		check( isHexVisible || !isActorInside );

		const TArray<FPoint> corners = FHexModel::PolygonCorners(layout, hex);

		posZ += isActorInside ? 3.0f : 0.0f;

		FColor Color = isActorInside ? FColor( 0, 255, 0 ) : FColor( 255, 0, 0 );
		if ( !isHexVisible )
		{
			Color = FColor( 155, 158, 163 );
			posZ += 3.0f;
		}

		static bool bPersistentLines = false;
		static float LifeTime = 0.0f;
		static uint8 DepthPriority = 0;
		const float Thickness = CVar_LineThickness.GetValueOnGameThread();

        FVector LineStart( 0.0f, 0.0f, posZ );
        FVector LineEnd( 0.0f, 0.0f, posZ );

		for(int i = 0; i < corners.Num( ) - 1; ++i)
		{
			const FPoint& coner = corners[ i ];
			const FPoint& conerNext = corners[ i + 1 ];

			LineStart.X = coner.X;
			LineStart.Y = coner.Y;

			LineEnd.X = conerNext.X;
			LineEnd.Y = conerNext.Y;

			DrawDebugLine( inWorld, LineStart, LineEnd, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		}

        const FPoint& coner = corners[ corners.Num( ) - 1 ];
        const FPoint& conerNext = corners[ 0 ];

		LineStart.X = coner.X;
        LineStart.Y = coner.Y;

        LineEnd.X = conerNext.X;
        LineEnd.Y = conerNext.Y;

        DrawDebugLine( inWorld, LineStart, LineEnd, Color, bPersistentLines, LifeTime, DepthPriority, Thickness );
	}

	void DrawCharacterPos( UWorld* inWorld )
	{
		check( inWorld );

        if( ACharacter* tmpCharacter = UGameplayStatics::GetPlayerCharacter( inWorld, 0 ) )
		{
			DrawDebugPoint( inWorld, tmpCharacter->GetActorLocation( ), 10.0f, FColor( 155, 0, 0 ), false, -1.0f, 100 );
		}
	}
}