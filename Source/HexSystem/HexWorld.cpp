// Fill out your copyright notice in the Description page of Project Settings.


#include "HexWorld.h"
#include "DrawDebugHelpers.h"

DECLARE_LOG_CATEGORY_EXTERN( LogTrace, Log, All );
DEFINE_LOG_CATEGORY( LogTrace );

namespace
{
	void DrawDebugHex( UWorld* inWorld, const FHexModel::FLayout& layout, const FHexModel::FHex& hex, float posZ );
}

// Sets default values
AHexWorld::AHexWorld()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

	UWorld* inWorld = GetWorld();

	for ( const auto& hex : HexMap )
	{
        DrawDebugHex( inWorld, FHexModel::FLayout( FHexModel::LayoutPointy,
                                                   FHexModel::FSize( 100, 100 ),
                                                   FHexModel::FOrigin( actorLoc.X, actorLoc.Y ) ),
					  hex,
                      actorLoc.Z );
	}
}

#ifdef WITH_EDITOR
bool AHexWorld::ShouldTickIfViewportsOnly( ) const
{
    return true;
}
#endif

namespace
{
	void DrawDebugHex( UWorld* inWorld, const FHexModel::FLayout& layout, const FHexModel::FHex& hex, float posZ )
	{
		if ( !inWorld )
		{
			return;
		}

		const TArray<FHexModel::FPoint> corners = FHexModel::PolygonCorners(layout, hex);

		static const FColor Color( 255, 0, 0 );
		static bool bPersistentLines = false;
		static float LifeTime = 0.0f;
		static uint8 DepthPriority = 0;
		static const float Thickness = 10;

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
}