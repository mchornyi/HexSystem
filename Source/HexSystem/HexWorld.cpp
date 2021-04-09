// Fill out your copyright notice in the Description page of Project Settings.


#include "HexWorld.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

DECLARE_LOG_CATEGORY_EXTERN( LogTrace, Log, All );
DEFINE_LOG_CATEGORY( LogTrace );

namespace
{
	void DrawDebugHex( UWorld* inWorld, const FHexModel::FLayout& layout, const FHexModel::FHex& hex, float posZ, bool isActorInside );
	void DrawCharacterPos( UWorld* inWorld );

    static TAutoConsoleVariable<float> CVar_HexSize( TEXT( "hex.HexSize" ), 100.0f, TEXT( "Defines the size of hex cell." ), ECVF_Default);
	static TAutoConsoleVariable<float> CVar_LineThickness( TEXT( "hex.LineThickness" ), 10.0f, TEXT( "Defines the hex line thickness for debug visualizing." ), ECVF_Default );
}

// Sets default values
AHexWorld::AHexWorld() :
	HexLayout(FHexModel::LayoutPointy, FHexModel::FSize( 100, 100 ), FHexModel::FOrigin( 0, 0 ) )
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    CVar_HexSize.AsVariable( )->SetOnChangedCallback( FConsoleVariableDelegate::CreateLambda( [ this ]( IConsoleVariable* Variable ){ UpdateHexLayout(); } ) );

	Generate( );
}

// Called when the game starts or when spawned
void AHexWorld::BeginPlay()
{
	Super::BeginPlay();

}

void AHexWorld::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Generate( );
}

void AHexWorld::Generate( )
{
	UE_LOG( LogTrace, Display, TEXT( __FUNCTION__ ) );

	FHexModel::GenerateHexMap( HexMap, 3 );
}

// Called every frame
void AHexWorld::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector& actorLoc = GetTransform( ).GetLocation( );
	HexLayout.origin = { actorLoc.X, actorLoc.Y };

	FHexModel::FHex currentHex{ 0, 0, 0 };
	if ( ACharacter* tmpCharacter = UGameplayStatics::GetPlayerCharacter( GetWorld( ), 0 ) )
	{
        const FHexModel::FFractionalHex fHex = FHexModel::PixelToHex( HexLayout, { tmpCharacter->GetActorLocation( ).X, tmpCharacter->GetActorLocation( ).Y } );
        currentHex = FHexModel::HexRound( fHex );
	}

	for ( const auto& hex : HexMap )
	{
        const bool isActorInside = (hex == currentHex);

        DrawDebugHex( GetWorld( ), HexLayout, hex, actorLoc.Z, isActorInside );
	}

	DrawCharacterPos( GetWorld( ) );
}

#ifdef WITH_EDITOR
bool AHexWorld::ShouldTickIfViewportsOnly( ) const
{
    return true;
}
#endif


void AHexWorld::UpdateHexLayout( )
{
    HexLayout.size = { CVar_HexSize.GetValueOnGameThread( ) , CVar_HexSize.GetValueOnGameThread( ) };
}

//void AHexWorld::OnChangeCVar( IConsoleVariable* var )
//{
//
//}

namespace
{
	void DrawDebugHex( UWorld* inWorld, const FHexModel::FLayout& layout, const FHexModel::FHex& hex, float posZ, bool isActorInside )
	{
		check( inWorld );

		const TArray<FHexModel::FPoint> corners = FHexModel::PolygonCorners(layout, hex);

		posZ += isActorInside ? 3.0f : 0.0f;

		const FColor Color = isActorInside ? FColor( 0, 255, 0 ) : FColor( 255, 0, 0 );
		static bool bPersistentLines = false;
		static float LifeTime = 0.0f;
		static uint8 DepthPriority = 0;
		const float Thickness = CVar_LineThickness.GetValueOnGameThread();

        FVector LineStart( 0.0f, 0.0f, posZ );
        FVector LineEnd( 0.0f, 0.0f, posZ );

		for(int i = 0; i < corners.Num( ) - 1; ++i)
		{
			const FHexModel::FPoint& coner = corners[ i ];
			const FHexModel::FPoint& conerNext = corners[ i + 1 ];

			LineStart.X = coner.X;
			LineStart.Y = coner.Y;

			LineEnd.X = conerNext.X;
			LineEnd.Y = conerNext.Y;

			DrawDebugLine( inWorld, LineStart, LineEnd, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		}

        const FHexModel::FPoint& coner = corners[ corners.Num( ) - 1 ];
        const FHexModel::FPoint& conerNext = corners[ 0 ];

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