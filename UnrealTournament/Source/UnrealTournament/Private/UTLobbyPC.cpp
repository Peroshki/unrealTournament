// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "GameFramework/PlayerController.h"
#include "UTLobbyGameMode.h"
#include "UTLobbyHUD.h"
#include "UTLocalPlayer.h"
#include "UTLobbyPlayerState.h"
#include "UTLobbyPC.h"
#include "UTLobbyMatchInfo.h"
#include "UTCharacterMovement.h"
#include "UTAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Analytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
#include "Online.h"
#include "UTOnlineGameSearchBase.h"
#include "OnlineSubsystemTypes.h"

AUTLobbyPC::AUTLobbyPC(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void AUTLobbyPC::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if ((GetNetMode() == NM_Client) && NetConnection && (NetConnection->NegotiatedVer != GEngineNetVersion))
	{
		// this should only happen if server is pre-official client
		UE_LOG(UT, Error, TEXT("Server is outdated:  local version %d negotiated from server %d"), GEngineNetVersion, NetConnection->NegotiatedVer);
	}
}

/* Cache a copy of the PlayerState cast'd to AUTPlayerState for easy reference.  Do it both here and when the replicated copy of APlayerState arrives in OnRep_PlayerState */
void AUTLobbyPC::InitPlayerState()
{
	Super::InitPlayerState();
	UTLobbyPlayerState = Cast<AUTLobbyPlayerState>(PlayerState);
}

void AUTLobbyPC::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	UTLobbyPlayerState = Cast<AUTLobbyPlayerState>(PlayerState);
}

void AUTLobbyPC::SetName(const FString& S)
{
}


void  AUTLobbyPC::ServerRestartPlayer_Implementation()
{
}

bool AUTLobbyPC::CanRestartPlayer()
{
	return false;
}


void AUTLobbyPC::MatchChat(FString Message)
{
	Chat(ChatDestinations::Match, Message);
}

void AUTLobbyPC::GlobalChat(FString Message)
{
	Chat(ChatDestinations::Global, Message);
}


void AUTLobbyPC::Chat(FName Destination, FString Message)
{
	// Send the Chat to the server so it can be routed to the right place.  
	// TODO - Once we have MCP friends support, look at routing friends chat directly through the MCP

	ServerChat(Destination, Message);
}

void AUTLobbyPC::ServerChat_Implementation(const FName Destination, const FString& Message)
{
	if (Destination == ChatDestinations::Match)
	{
		if (UTLobbyPlayerState && UTLobbyPlayerState->CurrentMatch)
		{
			UTLobbyPlayerState->CurrentMatch->BroadcastMatchMessage(UTLobbyPlayerState, Message);
		}
	}
	else if (Destination == ChatDestinations::Global)
	{
		AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
		if (LobbyGameState)
		{
			LobbyGameState->BroadcastMatchMessage(UTLobbyPlayerState, Message);

		}
	}


}

bool AUTLobbyPC::ServerChat_Validate(const FName Destination, const FString& Message)
{
	return true;
}

void AUTLobbyPC::ReceivedPlayer()
{
}
