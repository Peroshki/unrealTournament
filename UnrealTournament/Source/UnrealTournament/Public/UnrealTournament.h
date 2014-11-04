// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#ifndef __UNREALTOURNAMENT_H__
#define __UNREALTOURNAMENT_H__

#include "Engine.h"
#include "ParticleDefinitions.h"

DECLARE_LOG_CATEGORY_EXTERN(UT, Log, All);

#define COLLISION_PROJECTILE ECC_GameTraceChannel1
#define COLLISION_TRACE_WEAPON ECC_GameTraceChannel2
#define COLLISION_PROJECTILE_SHOOTABLE ECC_GameTraceChannel3
#define COLLISION_TELEPORTING_OBJECT ECC_GameTraceChannel4
#define COLLISION_PAWNOVERLAP ECC_GameTraceChannel5
#define COLLISION_TRACE_WEAPONNOCHARACTER ECC_GameTraceChannel6

#include "UTATypes.h"
#include "UTTeamInterface.h"
#include "UTResetInterface.h"
#include "UTGameplayStatics.h"
#include "UTGameUserSettings.h"
#include "UTLocalPlayer.h"
#include "UTBaseGameMode.h"
#include "UTLocalMessage.h"
#include "UTPlayerState.h"
#include "UTCharacter.h"
#include "UTBot.h"
#include "UTTeamInfo.h"
#include "UTGameState.h"
#include "UTHUD.h"
#include "UTHUDWidget.h"
#include "UTDamageType.h"
#include "UTBasePlayerController.h"
#include "UTPlayerController.h"
#include "UTProjectile.h"
#include "UTInventory.h"
#include "UTWeapon.h"
#include "UTPickup.h"
#include "UTGameSession.h"
#include "UTGameObjective.h"
#include "UTCarriedObjectMessage.h"
#include "UTCarriedObject.h"
#include "UTGameMode.h"
#include "UTTeamGameMode.h"

/** handy response params for world-only checks */
extern FCollisionResponseParams WorldResponseParams;

/** utility to find out if a particle system loops */
extern bool IsLoopingParticleSystem(const UParticleSystem* PSys);

/** utility to detach and unregister a component and all its children */
extern void UnregisterComponentTree(USceneComponent* Comp);

/** utility to retrieve the highest priority physics volume overlapping the passed in primitive */
extern APhysicsVolume* FindPhysicsVolume(UWorld* World, const FVector& TestLoc, const FCollisionShape& Shape);

/** workaround for FCanvasIcon not having a constructor you can pass in the values to */
FORCEINLINE FCanvasIcon MakeCanvasIcon(UTexture* Tex, float InU, float InV, float InUL, float InVL)
{
	FCanvasIcon Result;
	Result.Texture = Tex;
	Result.U = InU;
	Result.V = InV;
	Result.UL = InUL;
	Result.VL = InVL;
	return Result;
}

#endif
