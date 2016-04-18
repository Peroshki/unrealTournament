// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTAnnouncer.h"

#include "UTMultiKillMessage.generated.h"

UCLASS(CustomConstructor)
class UNREALTOURNAMENT_API UUTMultiKillMessage : public UUTLocalMessage
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	TArray<FText> AnnouncementText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	TArray<FName> AnnouncementNames;

	UUTMultiKillMessage(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		MessageArea = FName(TEXT("Announcements"));
		MessageSlot = FName(TEXT("MultiKill"));

		AnnouncementText.Add(NSLOCTEXT("UTMultiKillMessage", "AnnouncementText[0]", "Double Kill!"));
		AnnouncementText.Add(NSLOCTEXT("UTMultiKillMessage", "AnnouncementText[1]", "Multi Kill!"));
		AnnouncementText.Add(NSLOCTEXT("UTMultiKillMessage", "AnnouncementText[2]", "ULTRA KILL!"));
		AnnouncementText.Add(NSLOCTEXT("UTMultiKillMessage", "AnnouncementText[3]", "MONSTER KILL!"));
		AnnouncementNames.Add(TEXT("DoubleKill"));
		AnnouncementNames.Add(TEXT("MultiKill"));
		AnnouncementNames.Add(TEXT("UltraKill"));
		AnnouncementNames.Add(TEXT("MonsterKill"));

		bIsSpecial = true;
		bIsUnique = true;
		bIsConsoleMessage = false;
		Lifetime = 3.0f;
		bWantsBotReaction = true;
		ScaleInSize = 3.f;
	}

	virtual FLinearColor GetMessageColor_Implementation(int32 MessageIndex) const override
	{
		return FLinearColor::Red;
	}

	virtual bool ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const override
	{
		return IsLocalForAnnouncement(ClientData, true, true);
	}
	virtual FName GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject) const override
	{
		// Switch is MultiKillLevel - 1 (e.g. 0 is double kill)
		return AnnouncementNames.Num() > 0 ? AnnouncementNames[FMath::Clamp<int32>(Switch, 0, AnnouncementNames.Num() - 1)] : NAME_None;
	}
	virtual FText GetText(int32 Switch = 0, bool bTargetsPlayerState1 = false, class APlayerState* RelatedPlayerState_1 = NULL, class APlayerState* RelatedPlayerState_2 = NULL, class UObject* OptionalObject = NULL) const override
	{
		return AnnouncementText.Num() > 0 ? AnnouncementText[FMath::Clamp<int32>(Switch, 0, AnnouncementText.Num() - 1)] : FText();
	}

	virtual bool ShouldCountInstances_Implementation(int32 MessageIndex) const override
	{
		return MessageIndex == AnnouncementText.Num() - 1;
	}

};