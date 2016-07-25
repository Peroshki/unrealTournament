// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTProj_LinkPlasma.h"
#include "UTProjectileMovementComponent.h"
#include "UnrealNetwork.h"

AUTProj_LinkPlasma::AUTProj_LinkPlasma(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bNetTemporary = true;

	MaxSpeedPerLink = 700.f;
	ExtraScalePerLink = 0.25f;
	bLowPriorityLight = true;
	OverlapSphereGrowthRate = 40.0f;
	MaxOverlapSphereSize = 10.0f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AUTProj_LinkPlasma::SetLinks(int32 NewLinks)
{
	Links = NewLinks;
	LinksUpdated();
}

void AUTProj_LinkPlasma::LinksUpdated_Implementation()
{
	ProjectileMovement->MaxSpeed = GetClass()->GetDefaultObject<AUTProj_LinkPlasma>()->ProjectileMovement->MaxSpeed + MaxSpeedPerLink * Links;
	ProjectileMovement->InitialSpeed = GetClass()->GetDefaultObject<AUTProj_LinkPlasma>()->ProjectileMovement->InitialSpeed + MaxSpeedPerLink * Links;
	// if no accel projectile then apply to velocity immediately
	if (Cast<UUTProjectileMovementComponent>(ProjectileMovement) == NULL || ((UUTProjectileMovementComponent*)ProjectileMovement)->AccelRate <= 0.0f)
	{
		ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * ProjectileMovement->InitialSpeed;
	}
	RootComponent->SetWorldScale3D(GetClass()->GetDefaultObject<AUTProj_LinkPlasma>()->RootComponent->RelativeScale3D * (1.0f + (ExtraScalePerLink * Links)));
}

void AUTProj_LinkPlasma::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PawnOverlapSphere != nullptr && PawnOverlapSphere->GetUnscaledSphereRadius() < MaxOverlapSphereSize)
	{
		PawnOverlapSphere->SetSphereRadius(FMath::Min<float>(MaxOverlapSphereSize, PawnOverlapSphere->GetUnscaledSphereRadius() + OverlapSphereGrowthRate * DeltaTime));
	}
}

void AUTProj_LinkPlasma::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AUTProj_LinkPlasma, Links, COND_InitialOnly);
}
