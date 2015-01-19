// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTHUD_DM.h"
#include "UTDMGameMode.h"


AUTDMGameMode::AUTDMGameMode(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	HUDClass = AUTHUD_DM::StaticClass();
	DisplayName = NSLOCTEXT("UTGameMode", "DM", "Deathmatch");
}


void AUTDMGameMode::UpdateLobbyBadge()
{
	TArray<AUTPlayerState*> LeaderBoard;
	AUTPlayerState* Leader = NULL;
	AUTPlayerState* P = NULL;

	for (int i=0;i<UTGameState->PlayerArray.Num();i++)
	{
		P = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (P)
		{
			if (Leader == NULL || P->Score >= Leader->Score)
			{
				// Insert me and set as new leader.
				LeaderBoard.Insert(P,0);
				Leader = P;
			}
			else if (LeaderBoard.Num()<3)
			{
				LeaderBoard.Add(P);
			}
		}
	}

	FString LB = TEXT("");
	for (int i=0;i<LeaderBoard.Num() && i < 3; i++)
	{
		LB += FString::Printf(TEXT("<UWindows.Standard.MatchBadge>%s (%i)</>\n"), *LeaderBoard[i]->PlayerName, int(LeaderBoard[i]->Score));
	}

	FString Update = FString::Printf(TEXT("<UWindows.Standard.MatchBadge.Header>%s</>\n\n%s"), *DisplayName.ToString(), *LB);

	LobbyBeacon->Lobby_UpdateBadge(LobbyInstanceID, Update);

}
