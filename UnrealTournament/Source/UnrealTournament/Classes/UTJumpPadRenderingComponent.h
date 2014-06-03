// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTJumpPadRenderingComponent.generated.h"

UCLASS()
class UUTJumpPadRenderingComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;

	/** Should recreate proxy one very update */
	virtual bool ShouldRecreateProxyOnUpdateTransform() const OVERRIDE{ return true; }
	// End UPrimitiveComponent Interface

	// Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const OVERRIDE;
	// End USceneComponent Interface
};
