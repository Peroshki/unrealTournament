// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "GameFramework/GameMode.h"
#include "UTDeathMessage.h"
#include "UTGameMessage.h"
#include "UTVictoryMessage.h"

#include "UTTimedPowerup.h"
#include "UTCountDownMessage.h"

namespace MatchState
{
	const FName CountdownToBegin = FName(TEXT("CountdownToBegin"));
	const FName MatchEnteringOvertime = FName(TEXT("MatchEnteringOvertime"));
	const FName MatchIsInOvertime = FName(TEXT("MatchIsInOvertime"));
}


AUTGameMode::AUTGameMode(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// set default pawn class to our Blueprinted character

	static ConstructorHelpers::FObjectFinder<UBlueprint> PlayerPawnObject(TEXT("Blueprint'/Game/RestrictedAssets/Blueprints/WIP/Steve/SteveUTCharacter.SteveUTCharacter'"));
	
	if (PlayerPawnObject.Object != NULL)
	{
		DefaultPawnClass = (UClass*)PlayerPawnObject.Object->GeneratedClass;
	}

	// use our custom HUD class
	HUDClass = AUTHUD::StaticClass();

	GameStateClass = AUTGameState::StaticClass();
	PlayerStateClass = AUTPlayerState::StaticClass();

	PlayerControllerClass = AUTPlayerController::StaticClass();
	MinRespawnDelay = 1.5f;
	bUseSeamlessTravel = false;
	CountDown = 4;
	bPauseable = false;
	RespawnWaitTime = 0.0f;
	bPlayersMustBeReady = false;
	MinPlayersToStart = 1;
	EndScoreboardDelay = 2.0f;
	SpawnProtectionTime = 3.0f;
	VictoryMessageClass=UUTVictoryMessage::StaticClass();
	DeathMessageClass=UUTDeathMessage::StaticClass();
	GameMessageClass=UUTGameMessage::StaticClass();
}

// Parse options for this game...
void AUTGameMode::InitGame( const FString& MapName, const FString& Options, FString& ErrorMessage )
{
	Super::InitGame(MapName, Options, ErrorMessage);

	GameDifficulty = FMath::Max(0,GetIntOption(Options, TEXT("Difficulty"), GameDifficulty));

	FString InOpt = ParseOption(Options, TEXT("ForceRespawn"));
	bForceRespawn = EvalBoolOptions(InOpt, bForceRespawn);

	InOpt = ParseOption(Options, TEXT("MustBeReady"));
	bPlayersMustBeReady = EvalBoolOptions(InOpt, bPlayersMustBeReady);

	TimeLimit = FMath::Max(0,GetIntOption( Options, TEXT("TimeLimit"), TimeLimit ));

	// Set goal score to end match.
	GoalScore = FMath::Max(0,GetIntOption( Options, TEXT("GoalScore"), GoalScore ));

	MinPlayersToStart = FMath::Max(1, GetIntOption( Options, TEXT("MinPlayers"), MinPlayersToStart));

	RespawnWaitTime = FMath::Max(0,GetIntOption( Options, TEXT("RespawnWait"), RespawnWaitTime ));
	if (bForceRespawn) RespawnWaitTime = 0.0f;

}

void AUTGameMode::InitGameState()
{
	Super::InitGameState();

	UTGameState = Cast<AUTGameState>(GameState);
	if (UTGameState != NULL)
	{
		UTGameState->SetGoalScore(GoalScore);
		UTGameState->SetTimeLimit(0);
		UTGameState->RespawnWaitTime = RespawnWaitTime;
		UTGameState->bPlayerMustBeReady = bPlayersMustBeReady;
	}
	else
	{
		UE_LOG(UT,Error, TEXT("UTGameState is NULL %s"), *GameStateClass->GetFullName());
	}
}

/**
 *	DefaultTimer is called once per second and is useful for consistent timed events that don't require to be 
 *  done every frame.
 **/
void AUTGameMode::DefaultTimer()
{	
	// Let the game see if it's time to end the match
	CheckGameTime();
}


void AUTGameMode::Reset()
{
	Super::Reset();

	bGameEnded = false;

	//now respawn all the players
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AController* Controller = *Iterator;
		if (Controller->PlayerState != NULL && !Controller->PlayerState->bOnlySpectator)
		{
			RestartPlayer(Controller);
		}
	}

	UTGameState->SetTimeLimit(0);
}

void AUTGameMode::RestartGame()
{
	if (HasMatchStarted())
	{
		Super::RestartGame();
	}
}

bool AUTGameMode::IsEnemy(AController * First, AController* Second)
{
	// In DM - Everyone is an enemy
	return First != Second;
}

void AUTGameMode::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{

	// Ingore all killing when entering overtime as we kill off players and don't want it affecting their score.
	if (GetMatchState() == MatchState::MatchEnteringOvertime) return;	

	AUTPlayerState* const KillerPlayerState = Killer ? Cast<AUTPlayerState>(Killer->PlayerState) : NULL;
	AUTPlayerState* const KilledPlayerState = KilledPlayer ? Cast<AUTPlayerState>(KilledPlayer->PlayerState) : NULL;

	UE_LOG(UT,Log,TEXT("Player Killed: %s killed %s"), (KillerPlayerState != NULL ? *KillerPlayerState->PlayerName : TEXT("NULL")), (KilledPlayerState != NULL ? *KilledPlayerState->PlayerName : TEXT("NULL")));

	bool const bEnemyKill = IsEnemy(Killer, KilledPlayer);

	if ( KilledPlayerState != NULL )
	{
		KilledPlayerState->IncrementDeaths(KillerPlayerState);
		ScoreKill(Killer, KilledPlayer);
		BroadcastDeathMessage(Killer, KilledPlayer, DamageType);
	}

	DiscardInventory(KilledPawn, Killer);
	NotifyKilled(Killer, KilledPlayer, KilledPawn, DamageType);

	// Force Respawn 

	if (KilledPlayer && bForceRespawn)
	{
		RestartPlayer(KilledPlayer);
	}
}

void AUTGameMode::NotifyKilled(AController* Killer, AController* Killed, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
}

void AUTGameMode::ScoreKill(AController* Killer, AController* Other)
{
	if( (Killer == Other) || (Killer == NULL) )
	{
		// If it's a suicide, subtract a kill from the player...

		if (Other != NULL && Other->PlayerState != NULL && Cast<AUTPlayerState>(Other->PlayerState) != NULL)
		{
			Cast<AUTPlayerState>(Other->PlayerState)->AdjustScore(-1);
		}
	}
	else 
	{
		AUTPlayerState * KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
		if ( KillerPlayerState != NULL )
		{
			KillerPlayerState->AdjustScore(+1);
			KillerPlayerState->IncrementKills(true);
			CheckScore(KillerPlayerState);
		}
	}
}

void AUTGameMode::DiscardInventory(APawn* Other, AController* Killer)
{
	AUTCharacter* UTC = Cast<AUTCharacter>(Other);
	if (UTC != NULL)
	{
		// toss weapon
		if (UTC->GetWeapon() != NULL)
		{
			UTC->TossInventory(UTC->GetWeapon());
		}
		// toss all powerups
		AUTInventory* Inv = UTC->GetInventory();
		while (Inv != NULL)
		{
			AUTInventory* NextInv = Inv->GetNext();
			if (Inv->IsA(AUTTimedPowerup::StaticClass()))
			{
				UTC->TossInventory(Inv, FVector(FMath::FRandRange(0.0f, 200.0f), FMath::FRandRange(-400.0f, 400.0f), FMath::FRandRange(0.0f, 200.0f)));
			}
			Inv = NextInv;
		}
		// delete the rest
		UTC->DiscardAllInventory();
	}
}

bool AUTGameMode::CheckScore(AUTPlayerState* Scorer)
{
	if ( Scorer != NULL )
	{
		if ( (GoalScore > 0) && (Scorer->Score >= GoalScore) )
		{
			EndGame(Scorer,TEXT("fraglimit"));
		}
	}
	return true;
}


void AUTGameMode::StartMatch()
{
	if (HasMatchStarted())
	{
		// Already started
		return;
	}
	if (GetWorld()->IsPlayInEditor())
	{
		SetMatchState(MatchState::InProgress);
	}
	else
	{
		SetMatchState(MatchState::CountdownToBegin);	
	}
}

void AUTGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	UTGameState->SetTimeLimit(TimeLimit);

	BroadcastLocalized( this, UUTGameMessage::StaticClass(), 0, NULL, NULL, NULL);
}

void AUTGameMode::BeginGame()
{
	UE_LOG(UT,Log,TEXT("--------------------------"));
	UE_LOG(UT,Log,TEXT("Game Has Begun "));
	UE_LOG(UT,Log,TEXT("--------------------------"));

	UE_LOG(UT,Log,TEXT("GameType: %s"), *GetNameSafe(this));
	UE_LOG(UT,Log,TEXT("Difficulty: %i"), GameDifficulty);
	UE_LOG(UT,Log,TEXT("GoalScore: %i"), GoalScore);
	UE_LOG(UT,Log,TEXT("TimeLimit: %f"), TimeLimit);
	UE_LOG(UT,Log,TEXT("Min # of Players: %i"), MinPlayersToStart);
	UE_LOG(UT,Log,TEXT("End Delays %f / %f"), EndTimeDelay, EndScoreboardDelay);

	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* TestActor = *It;
		if (TestActor &&
			!TestActor->IsPendingKill() &&
			TestActor->IsA<APlayerState>())
		{
			Cast<APlayerState>(TestActor)->StartTime = 0;
		}
	}

	GameState->ElapsedTime = 0;

	//Let the game session override the StartMatch function, in case it wants to wait for arbitration

	if (GameSession->HandleStartMatchRequest())
	{
		return;
	}

	SetMatchState(MatchState::InProgress);
}

void AUTGameMode::EndMatch()
{
	Super::EndMatch();
	UTGameState->bStopCountdown = true;
	GetWorldTimerManager().SetTimer(this, &AUTGameMode::PlayEndOfMatchMessage, 1.0f);

	for (FConstPawnIterator Iterator = GetWorld()->GetPawnIterator(); Iterator; ++Iterator )
	{
		(*Iterator)->TurnOff();
	}
}

void AUTGameMode::EndGame(AUTPlayerState* Winner, const FString& Reason )
{

	if ( (FCString::Stricmp(*Reason, TEXT("triggered")) == 0) ||
		 (FCString::Stricmp(*Reason, TEXT("TimeLimit")) == 0) ||
		 (FCString::Stricmp(*Reason, TEXT("FragLimit")) == 0))
	{

		// If we don't have a winner, then go and find one
		if (Winner == NULL)
		{
			for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
			{
				AController* Controller = *Iterator;
				AUTPlayerState* CPS = Cast<AUTPlayerState> (Controller->PlayerState);
				if ( CPS && ((Winner == NULL) || (CPS->Score >= Winner->Score)) )
				{
					Winner = CPS;
				}
			}
		}

		UTGameState->WinnerPlayerState = Winner;
		EndTime = GetWorld()->TimeSeconds;

		SetEndGameFocus(Winner);

		// Allow replication to happen before reporting scores, stats, etc.
		GetWorldTimerManager().SetTimer(this, &AUTGameMode::HandleMatchHasEnded, 1.5f);
		bGameEnded = true;

		// Setup a timer to pop up the final scoreboard on everyone
		GetWorldTimerManager().SetTimer(this, &AUTGameMode::ShowFinalScoreboard, EndScoreboardDelay);

		// Setup a timer to continue to the next map.

		EndTime = GetWorld()->TimeSeconds;
		GetWorldTimerManager().SetTimer(this, &AUTGameMode::TravelToNextMap, EndTimeDelay);

		EndMatch();
	}
}

/**
 *	NOTE: This is a really simple map list.  It doesn't support multiple maps in the list, etc and is really dumb.  But it
 *  will work for now.
 **/
void AUTGameMode::TravelToNextMap()
{
	FString CurrentMapName = GetWorld()->GetMapName();

	int32 MapIndex = -1;
	for (int i=0;i<MapRotation.Num();i++)
	{
		if (MapRotation[i].EndsWith(CurrentMapName))
		{
			MapIndex = i;
			break;
		}
	}

	if (MapRotation.Num() > 0)
	{
		MapIndex = (MapIndex + 1) % MapRotation.Num();
		if (MapIndex >=0 && MapIndex < MapRotation.Num())
		{
			GetWorld()->ServerTravel(MapRotation[MapIndex],false);
			return;
		}
	}

	RestartGame();	
}

void AUTGameMode::ShowFinalScoreboard()
{
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != NULL)
		{
			PC->ClientToggleScoreboard(true);
		}
	}
}

void AUTGameMode::SetEndGameFocus(AUTPlayerState* Winner)
{
	EndGameFocus = Cast<AController>(Winner->GetOwner())->GetPawn();
	if ( (EndGameFocus == NULL) && (Cast<AController>(Winner->GetOwner()) != NULL) )
	{
		// If the controller of the winner does not have a pawn, give him one.
		RestartPlayer(Cast<AController>(Winner->GetOwner()));
		EndGameFocus = Cast<AController>(Winner->GetOwner())->GetPawn();
	}

	if ( EndGameFocus != NULL )
	{
		EndGameFocus->bAlwaysRelevant = true;
	}

	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AController* Controller = *Iterator;
		Controller->GameHasEnded(EndGameFocus, (Controller->PlayerState != NULL) && (Controller->PlayerState == Winner) );
	}
}


void AUTGameMode::BroadcastDeathMessage(AController* Killer, AController* Other, TSubclassOf<UDamageType> DamageType)
{
	if (DeathMessageClass != NULL)
	{
		if ( (Killer == Other) || (Killer == NULL) )
		{
			BroadcastLocalized(this, DeathMessageClass, 1, NULL, Other->PlayerState, DamageType);
		}
		else
		{
			BroadcastLocalized(this, DeathMessageClass, 0, Killer->PlayerState, Other->PlayerState, DamageType);
		}
	}
}


bool AUTGameMode::IsAWinner(AUTPlayerController* PC)
{
	return ( PC != NULL && ( PC->UTPlayerState->bOnlySpectator || PC->UTPlayerState == UTGameState->WinnerPlayerState));
}

void AUTGameMode::PlayEndOfMatchMessage()
{
	for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* Controller = *Iterator;
		if (Controller->IsA(AUTPlayerController::StaticClass()))
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(Controller);
			if ( (PC->PlayerState != NULL) && !PC->PlayerState->bOnlySpectator )
			{
				PC->ClientReceiveLocalizedMessage(VictoryMessageClass, IsAWinner(PC) ? 0 : 1);
			}
		}
	}
}

void AUTGameMode::RestartPlayer(AController* aPlayer)
{
	if ( !UTGameState->HasMatchStarted() && bPlayersMustBeReady )
	{
		// If we are in the pre-game stage then flag the player as ready to play.  The game starting will be handled in the DefaultTimer() event
		Cast<AUTPlayerState>(aPlayer->PlayerState)->bReadyToPlay = true;
		return;
	}

	if (!IsMatchInProgress())
	{
		return;
	}

	Super::RestartPlayer(aPlayer);
}

/* 
  Make sure pawn properties are back to default
  Also add default inventory
*/
void AUTGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);

	AUTCharacter* UTCharacter = Cast<AUTCharacter>(PlayerPawn);
	if ( UTCharacter != NULL && UTCharacter->GetInventory() == NULL )
	{
		UTCharacter->AddDefaultInventory(DefaultInventory);
	}
}

void AUTGameMode::ChangeName(AController* Other, const FString& S, bool bNameChange)
{
	// Cap player name's at 15 characters...
	FString SMod = S;
	if (SMod.Len()>15)
	{
		SMod = SMod.Left(15);
	}

    if ( !Other->PlayerState|| FCString::Stricmp(*Other->PlayerState->PlayerName, *SMod) == 0 )
    {
		return;
	}

	// Look to see if someone else is using the the new name
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AController* Controller = *Iterator;
		if (Controller->PlayerState && FCString::Stricmp(*Controller->PlayerState->PlayerName, *SMod) == 0)
		{
			if ( Cast<APlayerController>(Other) != NULL )
			{
					Cast<APlayerController>(Other)->ClientReceiveLocalizedMessage( GameMessageClass, 5 );
					if ( FCString::Stricmp(*Other->PlayerState->PlayerName, *DefaultPlayerName) == 0 )
					{
						Other->PlayerState->SetPlayerName(DefaultPlayerName+Other->PlayerState->PlayerId);
					}
				return;
			}
		}
	}

    Other->PlayerState->SetPlayerName(SMod);
}

bool AUTGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	if ( Player && Cast<APlayerStartPIE>(Player->StartSpot.Get()) )
	{
		return true;
	}

	return ( GetWorld()->GetNetMode() == NM_Standalone && Player != NULL && Player->StartSpot.IsValid() &&
		(GetMatchState() == MatchState::WaitingToStart || (Player->PlayerState != NULL && Cast<AUTPlayerState>(Player->PlayerState)->bWaitingPlayer))
		 && (RatePlayerStart(Cast<APlayerStart>(Player->StartSpot.Get()), Player) >= 0.f) );
}


AActor* AUTGameMode::FindPlayerStart(AController* Player, const FString& IncomingName)
{
	AActor* const Best = Super::FindPlayerStart(Player, IncomingName);
	if (Best)
	{
		LastStartSpot = Best;
	}

	return Best;
}

AActor* AUTGameMode::ChoosePlayerStart( AController* Player )
{
	// Start by choosing a random start
	int32 RandStart = FMath::RandHelper(PlayerStarts.Num());

	float BestRating = 0.f;
	APlayerStart* BestStart = NULL;
	for ( int32 i=RandStart; i<PlayerStarts.Num(); i++ )
	{
		APlayerStart* P = PlayerStarts[i];

		if (Cast<APlayerStartPIE>( P ) != NULL )
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			BestStart = P;
			break;
		}

		float NewRating = RatePlayerStart(P,Player);

		if ( NewRating >= 30 )
		{
			// this PlayerStart is good enough
			return P;
		}
		if ( NewRating > BestRating )
		{
			BestRating = NewRating;
			BestStart = P;
		}
	}
	for ( int32 i=0; i<RandStart; i++ )
	{
		APlayerStart* P = PlayerStarts[i];
		float NewRating = RatePlayerStart(P,Player);

		if ( NewRating >= 30 )
		{
			// this PlayerStart is good enough
			return P;
		}
		if ( NewRating > BestRating )
		{
			BestRating = NewRating;
			BestStart = P;
		}
	}
	return BestStart ? BestStart : Super::ChoosePlayerStart(Player);
}

float AUTGameMode::RatePlayerStart(APlayerStart* P, AController* Player)
{
	float Score = 30.0f;

	AActor* LastSpot = (Player != NULL && Player->StartSpot.IsValid()) ? Player->StartSpot.Get() : NULL;
	if (P == LastStartSpot || (LastSpot != NULL && P == LastSpot))
	{
		// avoid re-using starts
		Score -= 15.0f;
	}

	bool bTwoPlayerGame = (NumPlayers + NumBots == 2);

	if (Player != NULL)
	{
		FVector StartLoc = P->GetActorLocation();
		for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
		{
			AController* OtherController = *Iterator;
			ACharacter* OtherCharacter = Cast<ACharacter>( OtherController->GetPawn());

			if ( OtherCharacter && OtherCharacter->PlayerState )
			{
				if ( FMath::Abs(StartLoc.Z - OtherCharacter->GetActorLocation().Z) < P->CapsuleComponent->GetScaledCapsuleHalfHeight() + OtherCharacter->CapsuleComponent->GetScaledCapsuleHalfHeight()
					&& (StartLoc - OtherCharacter->GetActorLocation()).Size2D() < P->CapsuleComponent->GetScaledCapsuleRadius() + OtherCharacter->CapsuleComponent->GetScaledCapsuleRadius() )
				{
					// overlapping - would telefrag
					return -10.f;
				}

				float NextDist = (OtherCharacter->GetActorLocation() - StartLoc).Size();
				static FName NAME_RatePlayerStart = FName(TEXT("RatePlayerStart"));

				if ( NextDist < 3000.0f && 
					!GetWorld()->LineTraceTest(OtherCharacter->GetActorLocation() + FVector(0.f, 0.f, OtherCharacter->CapsuleComponent->GetScaledCapsuleHalfHeight()), StartLoc, ECC_Visibility, FCollisionQueryParams(NAME_RatePlayerStart, false, this)) )
				{
					Score -= (5.f - 0.001f * NextDist);
				}
				else if (bTwoPlayerGame)
				{
					// in 2 player game, look for any visibility
					Score += FMath::Min(2.f,0.001f*NextDist);
					if (!GetWorld()->LineTraceTest(OtherCharacter->GetActorLocation(), StartLoc, ECC_Visibility, FCollisionQueryParams(NAME_RatePlayerStart, false, this)))
					{
						Score -= 5.f;
					}
				}
			}
		}
	}
	return FMath::Max(Score, 0.2f);
}

/**
 *	We are going to duplicate GameMode's StartNewPlayer because we need to replicate the scoreboard class along with the hud class.  
 *  We are doing this here like this because we are trying to not change engine.  Ultimately the code to create the hud should be
 *  moved to it's own easy to override function instead of being hard-coded in StartNewPlayer
 **/
void AUTGameMode::StartNewPlayer(APlayerController* NewPlayer)
{
	AUTPlayerController* UTNewPlayer = Cast<AUTPlayerController>(NewPlayer);
	if (UTNewPlayer != NULL)
	{
		// tell client what hud class to use

		TSubclassOf<UUTScoreboard> ScoreboardClass = LoadClass<UUTScoreboard>(NULL, *ScoreboardClassName.ClassName, NULL, LOAD_None, NULL);
		UTNewPlayer->ClientSetHUDAndScoreboard(HUDClass, ScoreboardClass);

		if (!bDelayedStart)
		{
			// start match, or let player enter, immediately
			if (UTGameState->HasMatchStarted())
			{
				RestartPlayer(NewPlayer);
			}

			if (NewPlayer->GetPawn() != NULL)
			{
				NewPlayer->GetPawn()->ClientSetRotation(NewPlayer->GetPawn()->GetActorRotation());
			}
		}
	}
	else
	{
		Super::StartNewPlayer(NewPlayer);
	}
}

bool AUTGameMode::ReadyToStartMatch()
{
	// If bDelayed Start is set, wait for a manual match start
	if (bDelayedStart)
	{
		return false;
	}

	if (GetWorld()->IsPlayInEditor()) return true;	// PIE is always ready to start.

	// By default start when we have > 0 players
	if (GetMatchState() == MatchState::WaitingToStart)
	{
		if (NumPlayers + NumBots >= MinPlayersToStart)
		{
			if (bPlayersMustBeReady)
			{
				for (int i=0;i<UTGameState->PlayerArray.Num();i++)
				{
					AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
					if (PS != NULL && !PS->bOnlySpectator && !PS->bReadyToPlay)
					{
						return false;					
					}
				}
			}

			return true;
		}
	}
	return false;
}

/**
 *	Overwriting all of these functions to work the way I think it should.  Really, the match state should
 *  only be in 1 place otherwise it's prone to mismatch errors.  I'm chosen the GameState because it's
 *  replicated and will be available on clients.
 **/

bool AUTGameMode::HasMatchStarted() const
{
	return UTGameState->HasMatchStarted();
}

bool AUTGameMode::IsMatchInProgress() const
{
	return UTGameState->IsMatchInProgress();
}

bool AUTGameMode::HasMatchEnded() const
{
	return UTGameState->HasMatchEnded();
}

/**
 *	I needed to rework the ordering of SetMatchState until it can be corrected in the engine.
 **/
void AUTGameMode::SetMatchState(FName NewState)
{
	if (MatchState == NewState)
	{
		return;
	}

	MatchState = NewState;
	if (GameState)
	{
		GameState->SetMatchState(NewState);
	}

	// Call change callbacks

	if (MatchState == MatchState::WaitingToStart)
	{
		HandleMatchIsWaitingToStart();
	}
	else if (MatchState == MatchState::CountdownToBegin)
	{
		HandleCountdownToBegin();
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::WaitingPostMatch)
	{
		HandleMatchHasEnded();
	}
	else if (MatchState == MatchState::LeavingMap)
	{
		HandleLeavingMap();
	}
	else if (MatchState == MatchState::Aborted)
	{
		HandleMatchAborted();
	}
	else if (MatchState == MatchState::MatchEnteringOvertime)
	{
		HandleEnteringOvertime();
	}
	else if (MatchState == MatchState::MatchIsInOvertime)
	{
		HandleMatchInOvertime();
	}

}

void AUTGameMode::HandleEnteringOvertime()
{
	// We are entering overtime, kill off anyone not at the top of the leader board....

	AUTPlayerState* BestPlayer = NULL;
	AUTPlayerState* KillPlayer = NULL;
	float BestScore = 0.0;

	for (int PlayerIdx=0; PlayerIdx < UTGameState->PlayerArray.Num();PlayerIdx++)
	{
		if (UTGameState->PlayerArray[PlayerIdx] != NULL)
		{
			if (BestPlayer == NULL || UTGameState->PlayerArray[PlayerIdx]->Score > BestScore)
			{
				if (BestPlayer != NULL)
				{
					KillPlayer = BestPlayer;
				}
				BestPlayer = Cast<AUTPlayerState>(UTGameState->PlayerArray[PlayerIdx]);
				BestScore = BestPlayer->Score;
			}
			else if (UTGameState->PlayerArray[PlayerIdx]->Score < BestScore)
			{
				KillPlayer = Cast<AUTPlayerState>(UTGameState->PlayerArray[PlayerIdx]);
			}
		}

		if (KillPlayer != NULL)
		{
			// No longer the best.. kill him.. KILL HIM NOW!!!!!
			AController* COwner = Cast<AController> (KillPlayer->GetOwner());
			if (COwner != NULL && COwner->GetPawn() != NULL)
			{
				AUTCharacter* UTChar = Cast<AUTCharacter>(COwner->GetPawn());
				if (UTChar != NULL)
				{
					UE_LOG(UT,Log,TEXT("    -- Calling Died"));
					// Kill off the pawn...
					UTChar->Died(NULL,FDamageEvent(UUTDamageType::StaticClass()));
					// Send this character a message/taunt about not making the cut....
				}
			}

			KillPlayer = NULL;
		}
	}
	SetMatchState(MatchState::MatchIsInOvertime);
}

void AUTGameMode::HandleMatchInOvertime()
{
	// Send the overtime message....
	BroadcastLocalized( this, UUTGameMessage::StaticClass(), 1, NULL, NULL, NULL);
}

void AUTGameMode::HandleCountdownToBegin()
{
	CountDown = 5;
	CheckCountDown();
}

void AUTGameMode::CheckCountDown()
{
	if (CountDown >0)
	{
		// Broadcast the localized message saying when the game begins.
		BroadcastLocalized( this, UUTCountDownMessage::StaticClass(), CountDown, NULL, NULL, NULL);
		GetWorldTimerManager().SetTimer(this, &AUTGameMode::CheckCountDown, 1.0,false);
		CountDown--;
	}
	else
	{
		BeginGame();
	}
}

void AUTGameMode::CheckGameTime()
{
	if ( IsMatchInProgress() && !HasMatchEnded() && TimeLimit > 0 && UTGameState->RemainingTime <= 0)
	{
		// Game should be over.. look to see if we need to go in to overtime....	

		uint32 bTied = 0;
		AUTPlayerState* Winner = IsThereAWinner(bTied);

		if (!bAllowOvertime || !bTied)
		{
			EndGame(Winner, TEXT("TimeLimit"));			
		}
		else if (bAllowOvertime && !UTGameState->IsMatchInOvertime())
		{
			SetMatchState(MatchState::MatchEnteringOvertime);
		}
	}
}

/**
 *	Look though the player states and see if we have a winner.  If there is a tie, we return
 *  NULL so that we can enter overtime.
 **/
AUTPlayerState* AUTGameMode::IsThereAWinner(uint32& bTied)
{
	AUTPlayerState* BestPlayer = NULL;
	float BestScore = 0.0;

	for (int PlayerIdx=0; PlayerIdx < UTGameState->PlayerArray.Num();PlayerIdx++)
	{
		if (UTGameState->PlayerArray[PlayerIdx] != NULL)
		{
			if (BestPlayer == NULL || UTGameState->PlayerArray[PlayerIdx]->Score > BestScore)
			{
				BestPlayer = Cast<AUTPlayerState>(UTGameState->PlayerArray[PlayerIdx]);
				BestScore = BestPlayer->Score;
				bTied = 0;
			}
			else if (UTGameState->PlayerArray[PlayerIdx]->Score == BestScore)
			{
				bTied = 1;
			}
		}
	}

	return BestPlayer;
}

bool AUTGameMode::PlayerCanRestart( APlayerController* Player )
{
	// Can't restart in overtime
	if (UTGameState->IsMatchInOvertime())
	{
		return false;
	}

	return Super::PlayerCanRestart(Player);
}

void AUTGameMode::ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FDamageEvent& DamageEvent, AActor* DamageCauser)
{
	AUTCharacter* InjuredChar = Cast<AUTCharacter>(Injured);
	if (InjuredChar != NULL && InjuredChar->bSpawnProtectionEligible && GetWorld()->TimeSeconds - Injured->CreationTime < SpawnProtectionTime)
	{
		Damage = 0;
	}
}
