// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "Slate/SUTInGameMenu.h"
#include "UTGameEngine.h"
#include "UTGameInstance.h"
#include "DataChannel.h"

AUTBaseGameMode::AUTBaseGameMode(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void AUTBaseGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &AUTBaseGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation() / GetWorldSettings()->DemoPlayTimeDilation, true);
}

void AUTBaseGameMode::CreateServerID()
{
	// Create a server instance id for this server and save it out to the config file
	ServerInstanceGUID = FGuid::NewGuid();
	ServerInstanceID = ServerInstanceGUID.ToString();
	SaveConfig();
}

void AUTBaseGameMode::InitGame( const FString& MapName, const FString& Options, FString& ErrorMessage )
{
	if (!PlayerPawnObject.IsNull())
	{
		DefaultPawnClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *PlayerPawnObject.ToStringReference().AssetLongPathname, NULL, LOAD_NoWarn));
	}

	// Grab the InstanceID if it's there.
	LobbyInstanceID = GetIntOption( Options, TEXT("InstanceID"), 0);

	// If we are a lobby instance, then we always want to generate a ServerInstanceID
	if (LobbyInstanceID > 0)
	{
		ServerInstanceGUID = FGuid::NewGuid();
	}
	else   // Otherwise, we want to try and load our instance id from the config so we are consistent.
	{
		if (ServerInstanceID.IsEmpty())
		{
			CreateServerID();
		}
		else
		{
			if ( !FGuid::Parse(ServerInstanceID, ServerInstanceGUID) )
			{
				UE_LOG(UT,Log,TEXT("WARNING: Could to import this server's previous ID.  A new one has been created so older links to this server will not work."));
				CreateServerID();
			}
		}
	}


	Super::InitGame(MapName, Options, ErrorMessage);

	if (HasOption(Options, TEXT("ServerPassword")))
	{
		ServerPassword = ParseOption(Options, TEXT("ServerPassword"));
	}
	if (HasOption(Options, TEXT("SpectatePassword")))
	{
		SpectatePassword = ParseOption(Options, TEXT("SpectatePassword"));
	}

	bRequirePassword = !ServerPassword.IsEmpty() || !SpectatePassword.IsEmpty();
	bTrainingGround = EvalBoolOptions(ParseOption(Options, TEXT("TG")), bTrainingGround);

	if (bTrainingGround)
	{
		UE_LOG(UT,Log,TEXT("=== This is a Training Ground Server.  It will only be visibly to beginners ==="));
	}

	
	UE_LOG(UT,Log,TEXT("Password: %i %s"), bRequirePassword, ServerPassword.IsEmpty() ? TEXT("NONE") : *ServerPassword)
}

FName AUTBaseGameMode::GetNextChatDestination(AUTPlayerState* PlayerState, FName CurrentChatDestination)
{
	if (CurrentChatDestination == ChatDestinations::Local) return ChatDestinations::Team;
	if (CurrentChatDestination == ChatDestinations::Team)
	{
		if (IsGameInstanceServer())
		{
			return ChatDestinations::Lobby;
		}
	}

	return ChatDestinations::Local;
}

void AUTBaseGameMode::GetInstanceData(TArray<TSharedPtr<FServerInstanceData>>& InstanceData)
{
}

int32 AUTBaseGameMode::GetNumPlayers()
{
	return NumPlayers;
}


int32 AUTBaseGameMode::GetNumMatches()
{
	return 1;
}

void AUTBaseGameMode::PreLogin(const FString& Options, const FString& Address, const TSharedPtr<class FUniqueNetId>& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	// Allow our game session to validate that a player can play
	AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
	if (ErrorMessage.IsEmpty() && UTGameSession)
	{
		UTGameSession->ValidatePlayer(Address, UniqueId, ErrorMessage);
	}

	if (ErrorMessage.IsEmpty() && UniqueId.IsValid())
	{
		// precache the user's entitlements now so that we'll hopefully have them by the time they get in
		if (IOnlineSubsystem::Get() != NULL)
		{
			IOnlineEntitlementsPtr EntitlementInterface = IOnlineSubsystem::Get()->GetEntitlementsInterface();
			if (EntitlementInterface.IsValid())
			{
				EntitlementInterface->QueryEntitlements(*UniqueId.Get(), TEXT("ut"));
			}
		}
	}
}

APlayerController* AUTBaseGameMode::Login(class UPlayer* NewPlayer, ENetRole RemoteRole, const FString& Portal, const FString& Options, const TSharedPtr<class FUniqueNetId>& UniqueId, FString& ErrorMessage)
{
	// local players don't go through PreLogin()
	if (UniqueId.IsValid() && Cast<ULocalPlayer>(NewPlayer) != NULL && IOnlineSubsystem::Get() != NULL)
	{
		IOnlineEntitlementsPtr EntitlementInterface = IOnlineSubsystem::Get()->GetEntitlementsInterface();
		if (EntitlementInterface.IsValid())
		{
			// note that we need to redundantly query even if we already got this user's entitlements because they might have quit, bought some stuff, then come back
			EntitlementInterface->QueryEntitlements(*UniqueId.Get(), TEXT("ut"));
		}
	}

	return Super::Login(NewPlayer, RemoteRole, Portal, Options, UniqueId, ErrorMessage);
}

void AUTBaseGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	FString CloudID = GetCloudID();

	APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
	AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(NewPlayer);
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (NewPlayer != LocalPC && PC && UTEngine)
	{
		PC->ClientRequireContentItemListBegin(CloudID);
		for (auto It = UTEngine->LocalContentChecksums.CreateConstIterator(); It; ++It)
		{
			PC->ClientRequireContentItem(It.Key(), It.Value());
		}
		PC->ClientRequireContentItemListComplete();
	}
}

void AUTBaseGameMode::GenericPlayerInitialization(AController* C)
{
	AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(C);
	if (PC)
	{
		PC->ClientGenericInitialization();
	}
}

bool AUTBaseGameMode::FindRedirect(const FString& PackageName, FPackageRedirectReference& Redirect)
{
	FString BasePackageName = FPaths::GetBaseFilename(PackageName);
	for (int32 i = 0; i < RedirectReferences.Num(); i++)
	{
		FString BaseRedirectPackageName = FPaths::GetBaseFilename(RedirectReferences[i].PackageName);
		if (BasePackageName.Equals(BaseRedirectPackageName,ESearchCase::IgnoreCase))
		{
			Redirect = RedirectReferences[i];
			return true;
		}
	}
	return false;
}

void AUTBaseGameMode::GatherRequiredRedirects(TArray<FPackageRedirectReference>& Redirects)
{
	// map pak
	FPackageRedirectReference Redirect;
	if (FindRedirect(GetModPakFilenameFromPkg(GetOutermost()->GetName()), Redirect))
	{
		Redirects.Add(Redirect);
	}
	// game class pak
	if (FindRedirect(GetModPakFilenameFromPkg(GetClass()->GetOutermost()->GetName()), Redirect))
	{
		Redirects.Add(Redirect);
	}
}

void AUTBaseGameMode::GameWelcomePlayer(UNetConnection* Connection, FString& RedirectURL)
{
	TArray<FPackageRedirectReference> AllRedirects;
	GatherRequiredRedirects(AllRedirects);

	uint8 MessageType = UNMT_Redirect;
	for (const FPackageRedirectReference& Redirect : AllRedirects)
	{
		FString RedirectInfo = FString::Printf(TEXT("%s\n%s\n%s"), *Redirect.PackageName, *Redirect.ToString(), *Redirect.PackageChecksum);
		FNetControlMessage<NMT_GameSpecific>::Send(Connection, MessageType, RedirectInfo);
	}

	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine != NULL) // in PIE this will happen
	{
		FString PackageName = Connection->ClientWorldPackageName.ToString();
		FString PackageBaseFilename = FPaths::GetBaseFilename(PackageName) + TEXT("-WindowsNoEditor");

		FString PackageChecksum;
		for (auto It = UTEngine->LocalContentChecksums.CreateConstIterator(); It; ++It)
		{
			if (It.Key() == PackageBaseFilename)
			{
				PackageChecksum = It.Value();
			}
		}

		for (int32 i = 0; i < RedirectReferences.Num(); i++)
		{
			if (RedirectReferences[i].PackageName == PackageBaseFilename)
			{
				FPackageRedirectReference R = RedirectReferences[i];
				RedirectURL = R.ToString() + PackageChecksum;
				return;
			}
		}

		FString CloudID = GetCloudID();
		if (!CloudID.IsEmpty() && !PackageChecksum.IsEmpty())
		{
			FString BaseURL = TEXT("https://ut-public-service-prod10.ol.epicgames.com/ut/api/cloudstorage/user/");
			FString McpConfigOverride;
			FParse::Value(FCommandLine::Get(), TEXT("MCPCONFIG="), McpConfigOverride);
			if (McpConfigOverride == TEXT("gamedev"))
			{
				BaseURL = TEXT("https://ut-public-service-gamedev.ol.epicgames.net/ut/api/cloudstorage/user/");
			}

			RedirectURL = BaseURL + GetCloudID() + TEXT("/") + PackageBaseFilename + TEXT(".pak") + TEXT(" ") + PackageChecksum;
		}
	}
}

FString AUTBaseGameMode::GetCloudID() const
{
	FString CloudID;

	APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());

	// For dedicated server, will need to pass stats id as a commandline parameter
	if (!FParse::Value(FCommandLine::Get(), TEXT("CloudID="), CloudID))
	{
		if (LocalPC)
		{
			UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(LocalPC->Player);
			IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
			if (OnlineSubsystem && LP)
			{
				IOnlineIdentityPtr OnlineIdentityInterface = OnlineSubsystem->GetIdentityInterface();
				if (OnlineIdentityInterface.IsValid())
				{
					TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(LP->GetControllerId());
					if (UserId.IsValid())
					{
						CloudID = UserId->ToString();
					}
				}
			}
		}
	}

	return CloudID;
}

#if !UE_SERVER
/**
 *	Returns the Menu to popup when the user requests a menu
 **/
TSharedRef<SUWindowsDesktop> AUTBaseGameMode::GetGameMenu(UUTLocalPlayer* PlayerOwner) const
{
	return SNew(SUTInGameMenu).PlayerOwner(PlayerOwner);
}
#endif

void AUTBaseGameMode::SendRconMessage(const FString& DestinationId, const FString &Message)
{	
	AUTGameState* UTGameState = Cast<AUTGameState>(GameState);
	if (UTGameState)
	{
		for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
		{
			if ( DestinationId == TEXT("") || UTGameState->PlayerArray[i]->UniqueId.ToString() == DestinationId || UTGameState->PlayerArray[i]->PlayerName.Equals(DestinationId,ESearchCase::IgnoreCase) )
			{			
				AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
				if (UTPlayerState)
				{
					UTPlayerState->ClientReceiveRconMessage(Message);
				}
			}
		}
	}
}

void AUTBaseGameMode::RconKick(const FString& NameOrUIDStr, bool bBan, const FString& Reason)
{
	AGameState* GS = GetWorld()->GetGameState<AGameState>();
	AGameSession* GSession = GameSession;
	if (GS && GSession)
	{
		for (int32 i=0; i < GS->PlayerArray.Num(); i++)
		{
			if ( (GS->PlayerArray[i]->PlayerName.ToLower() == NameOrUIDStr.ToLower()) ||
				 (GS->PlayerArray[i]->UniqueId.ToString() == NameOrUIDStr))
			{
				APlayerController* PC = Cast<APlayerController>(GS->PlayerArray[i]->GetOwner());
				if (PC)
				{
					if (bBan)
					{
						GSession->BanPlayer(PC,FText::FromString(Reason));
					}
					else
					{
						GSession->KickPlayer(PC, FText::FromString(Reason));
					}
				}
			}
		}
	}
}

void AUTBaseGameMode::RconAuth(AUTBasePlayerController* Admin, const FString& Password)
{
	if (Admin)
	{
		if (Admin->UTPlayerState && !Admin->UTPlayerState->bIsRconAdmin && !GetDefault<UUTGameEngine>()->RconPassword.IsEmpty())
		{
			if (GetDefault<UUTGameEngine>()->RconPassword.Equals(Password, ESearchCase::CaseSensitive))
			{
				Admin->ClientSay(Admin->UTPlayerState, TEXT("Rcon authenticated!"), ChatDestinations::System);
				Admin->UTPlayerState->bIsRconAdmin = true;
				return;
			}
		}

		Admin->ClientSay(Admin->UTPlayerState, TEXT("Rcon password incorrect or unset"), ChatDestinations::System);
	}

}

void AUTBaseGameMode::RconNormal(AUTBasePlayerController* Admin)
{
	if (Admin && Admin->UTPlayerState->bIsRconAdmin)
	{
		Admin->ClientSay(Admin->UTPlayerState, TEXT("Rcon status removed!"), ChatDestinations::System);
		Admin->UTPlayerState->bIsRconAdmin = false;
	}
}
