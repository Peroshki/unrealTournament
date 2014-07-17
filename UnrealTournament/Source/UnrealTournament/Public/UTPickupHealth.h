// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPickupHealth.generated.h"

UCLASS(Blueprintable, Abstract)
class AUTPickupHealth : public AUTPickup
{
	GENERATED_UCLASS_BODY()

	/** amount of health to restore */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	int32 HealAmount;
	/** if true, heal amount goes to SuperHealthMax instead of HealthMax */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	bool bSuperHeal;

	/** return the upper limit this pickup can increase the character's health to */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Pickup)
	int32 GetHealMax(AUTCharacter* P);

	virtual void ProcessTouch_Implementation(APawn* TouchedBy) override;
	virtual void GiveTo_Implementation(APawn* Target) override;
};