// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWaterVolume.h"
#include "UTPainVolume.h"

AUTWaterVolume::AUTWaterVolume(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	bWaterVolume = true;
	FluidFriction = 0.3f;
	PawnEntryVelZScaling = 0.4f;
	BrakingDecelerationSwimming = 300.f;
	TerminalVelocity = 3000.f;

}

AUTPainVolume::AUTPainVolume(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	bWaterVolume = true;
	FluidFriction = 0.5f;
	PawnEntryVelZScaling = 0.4f;
	BrakingDecelerationSwimming = 2000.f;
	TerminalVelocity = 3000.f;
}

void AUTWaterVolume::ActorEnteredVolume(class AActor* Other)
{
	if (Other)
	{
		if (EntrySound)
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), EntrySound, Other, SRT_None);
		}
		ACharacter* P = Cast<ACharacter>(Other);
		if (P && P->CharacterMovement)
		{
			P->CharacterMovement->Velocity.Z *= PawnEntryVelZScaling;
			P->CharacterMovement->BrakingDecelerationSwimming = BrakingDecelerationSwimming;
		}
		Super::ActorEnteredVolume(Other);
	}
}

void AUTWaterVolume::ActorLeavingVolume(class AActor* Other)
{
	if (Other)
	{
		if (ExitSound)
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), ExitSound, Other, SRT_None);
		}
		Super::ActorLeavingVolume(Other);
	}
}

void AUTPainVolume::ActorEnteredVolume(class AActor* Other)
{
	if (Other)
	{
		if (EntrySound)
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), EntrySound, Other, SRT_None);
		}
		ACharacter* P = Cast<ACharacter>(Other);
		if (P && P->CharacterMovement)
		{
			P->CharacterMovement->Velocity.Z *= PawnEntryVelZScaling;
			P->CharacterMovement->BrakingDecelerationSwimming = BrakingDecelerationSwimming;
		}
		Super::ActorEnteredVolume(Other);
	}
}

void AUTPainVolume::ActorLeavingVolume(class AActor* Other)
{
	if (Other)
	{
		if (ExitSound)
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), ExitSound, Other, SRT_None);
		}
		Super::ActorLeavingVolume(Other);
	}
}


