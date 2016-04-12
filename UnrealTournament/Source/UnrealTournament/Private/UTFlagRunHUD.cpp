// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTFlagRunHUD.h"
#include "UTCTFGameState.h"
#include "UTCTFGameMode.h"
#include "UTCTFScoreboard.h"

AUTFlagRunHUD::AUTFlagRunHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bDrawMinimap = true;

	ConstructorHelpers::FObjectFinder<UTexture2D> PlayerStartTextureObject(TEXT("/Game/RestrictedAssets/UI/MiniMap/minimap_atlas.minimap_atlas"));
	PlayerStartIcon.U = 128;
	PlayerStartIcon.V = 192;
	PlayerStartIcon.UL = 64;
	PlayerStartIcon.VL = 64;
	PlayerStartIcon.Texture = PlayerStartTextureObject.Object;
}

void AUTFlagRunHUD::DrawHUD()
{
	Super::DrawHUD();

	AUTCTFGameState* GS = GetWorld()->GetGameState<AUTCTFGameState>();
	if (!bShowScores && GS && GS->GetMatchState() == MatchState::InProgress && GS->bAsymmetricVictoryConditions)
	{
		// draw pips for players alive on each team @TODO move to widget
		TArray<AUTPlayerState*> LivePlayers;
		int32 OldRedCount = RedPlayerCount;
		int32 OldBlueCount = BluePlayerCount;
		RedPlayerCount = 0;
		BluePlayerCount = 0;
		float XAdjust = 0.015f * Canvas->ClipX * GetHUDWidgetScaleOverride();
		float PipSize = 0.02f * Canvas->ClipX * GetHUDWidgetScaleOverride();
		float SkullPipSize = PipSize * 0.6f;
		float XOffsetRed = 0.5f * Canvas->ClipX - XAdjust - PipSize;
		float XOffsetBlue = 0.5f * Canvas->ClipX + XAdjust;
		float YOffset = 0.1f * Canvas->ClipY * GetHUDWidgetScaleOverride();
		float XOffsetText = 0.f;
		FFontRenderInfo TextRenderInfo;
		TextRenderInfo.bEnableShadow = true;
		for (APlayerState* PS : GS->PlayerArray)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(PS);
			if (UTPS != NULL && UTPS->Team != NULL && !UTPS->bOnlySpectator  && !UTPS->bIsInactive)
			{
				bool bIsAttacker = (GS->bRedToCap == (UTPS->Team->TeamIndex == 0));
				if (bIsAttacker ? GS->bAttackerLivesLimited : GS->bDefenderLivesLimited)
				{
					if (!UTPS->bOutOfLives)
					{
						bool bLastLife = (UTPS->RemainingLives == 0);
						if (UTPS->Team->TeamIndex == 0)
						{
							RedPlayerCount++;
							Canvas->SetLinearDrawColor(bLastLife ? FLinearColor::Yellow : FLinearColor::Red, 0.7f);
							Canvas->DrawTile(PlayerStartIcon.Texture, XOffsetRed, YOffset, PipSize, PipSize, PlayerStartIcon.U, PlayerStartIcon.V, PlayerStartIcon.UL, PlayerStartIcon.VL, BLEND_Translucent);
							XOffsetText = XOffsetRed;
							XOffsetRed -= 1.1f*PipSize;
						}
						else
						{
							BluePlayerCount++;
							Canvas->SetLinearDrawColor(bLastLife ? FLinearColor::Yellow : FLinearColor::Blue, 0.7f);
							Canvas->DrawTile(PlayerStartIcon.Texture, XOffsetBlue, YOffset, PipSize, PipSize, PlayerStartIcon.U, PlayerStartIcon.V, PlayerStartIcon.UL, PlayerStartIcon.VL, BLEND_Translucent);
							XOffsetText = XOffsetBlue;
							XOffsetBlue += 1.1f*PipSize;
						}
						if (!bLastLife)
						{
							Canvas->SetLinearDrawColor(FLinearColor::White, 1.f);
							Canvas->DrawText(TinyFont, FText::AsNumber(UTPS->RemainingLives), XOffsetText + 0.4f*PipSize, YOffset, 0.5f, 0.5f, TextRenderInfo);
						}
					}
				}
				else
				{
					// @todo fixmesteve optimize - one pass through pawns, set the value of cachedcharacter, show those with none or dead
					AUTCharacter* Character = UTPS->GetUTCharacter();
					Canvas->SetLinearDrawColor(FLinearColor::White, 0.7f);
					if (!Character || Character->IsDead())
					{
						if (UTPS->Team->TeamIndex == 0)
						{
							Canvas->DrawTile(HUDAtlas, XOffsetRed, YOffset, SkullPipSize, SkullPipSize, 725, 0, 28, 36, BLEND_Translucent);
							XOffsetText = XOffsetRed;
							XOffsetRed -= 1.8f*SkullPipSize;
						}
						else
						{
							Canvas->DrawTile(HUDAtlas, XOffsetBlue, YOffset, SkullPipSize, SkullPipSize, 725, 0, 28, 36, BLEND_Translucent);
							XOffsetText = XOffsetBlue;
							XOffsetBlue += 1.8f*SkullPipSize;
						}
						Canvas->SetLinearDrawColor(FLinearColor::White, 1.f);
						Canvas->DrawText(TinyFont, FText::AsNumber(int32(UTPS->RespawnTime + 1.f)), XOffsetText + 0.4f*SkullPipSize, YOffset, 0.5f, 0.5f, TextRenderInfo);
					}
				}
			}
		}
		if (OldRedCount > RedPlayerCount)
		{
			RedDeathTime = GetWorld()->GetTimeSeconds();
		}
		if (OldBlueCount > BluePlayerCount)
		{
			BlueDeathTime = GetWorld()->GetTimeSeconds();
		}

		float TimeSinceRedDeath = GetWorld()->GetTimeSeconds() - RedDeathTime;
		if (TimeSinceRedDeath < 0.5f)
		{
			Canvas->SetLinearDrawColor(FLinearColor::Red, 0.5f - TimeSinceRedDeath);
			float ScaledSize = 1.f + 2.f*TimeSinceRedDeath;
			Canvas->DrawTile(PlayerStartIcon.Texture, XOffsetRed - 0.5f*(ScaledSize - 1.f)*PipSize, YOffset - 0.5f*(ScaledSize - 1.f)*PipSize, PipSize, PipSize, PlayerStartIcon.U, PlayerStartIcon.V, PlayerStartIcon.UL, PlayerStartIcon.VL, BLEND_Translucent);
		}

		float TimeSinceBlueDeath = GetWorld()->GetTimeSeconds() - BlueDeathTime;
		if (TimeSinceBlueDeath < 0.5f)
		{
			Canvas->SetLinearDrawColor(FLinearColor::Blue, 0.5f - TimeSinceBlueDeath);
			float ScaledSize = 1.f + 2.f*TimeSinceBlueDeath;
			Canvas->DrawTile(PlayerStartIcon.Texture, XOffsetBlue - 0.5f*(ScaledSize - 1.f)*PipSize, YOffset - 0.5f*(ScaledSize - 1.f)*PipSize, PipSize, PipSize, PlayerStartIcon.U, PlayerStartIcon.V, PlayerStartIcon.UL, PlayerStartIcon.VL, BLEND_Translucent);
		}
	}
}