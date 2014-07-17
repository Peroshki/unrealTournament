// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTGameMode.generated.h"

/** Defines the current state of the game. */

namespace MatchState
{
	extern const FName CountdownToBegin;				// We are entering this map, actors are not yet ticking
	extern const FName MatchEnteringOvertime;			// The game is entering overtime
	extern const FName MatchIsInOvertime;				// The game is in overtime
}


UCLASS(MinimalAPI, Config = Game, Abstract)
class AUTGameMode : public AGameMode
{
	GENERATED_UCLASS_BODY()

public:
	/** Cached reference to our game state for quick access. */
	UPROPERTY()
	AUTGameState* UTGameState;		

	/** Currently not used, but will be needed later*/
	UPROPERTY(globalconfig)
	float GameDifficulty;		

	/** How long to wait after the end of a match before the transition to the new level start */
	UPROPERTY(globalconfig)
	float EndTimeDelay;			

	/* How long after the end of the match before we display the scoreboard */
	UPROPERTY(globalconfig)
	float EndScoreboardDelay;			

	UPROPERTY(config)
	uint32 bAllowOvertime:1;

	/** If TRUE, force dead players to respawn immediately */
	UPROPERTY(globalconfig)
	uint32 bForceRespawn:1;		

	/** Score needed to win the match.  Can be overridden with GOALSCORE=x on the url */
	UPROPERTY(config)
	int32 GoalScore;    

	/** How long should the match be.  Can be overridden with TIMELIMIT=x on the url */
	UPROPERTY(config)
	int32 TimeLimit;    

	/** Will be TRUE if the game has ended */
	UPROPERTY()
	uint32 bGameEnded:1;    

	/** Will be TRUE if this is a team game */
	UPROPERTY()
	uint32 bTeamGame:1;

	/** If true, players will have to all be ready before the match will begin */
	UPROPERTY()
	uint32 bPlayersMustBeReady;

	UPROPERTY()
	int32 MinPlayersToStart;

	// How long a player must wait before respawning.  Set to 0 for no delay.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Rules)
	uint32 RespawnWaitTime;

	/** TRUE if we have started the count down to the match starting */
	UPROPERTY()
	uint32 bStartedCountDown:1;

	/** # of seconds before the match begins */
	UPROPERTY()
	int32 CountDown;

	/** Holds the last place any player started from */
	UPROPERTY()
	class AActor* LastStartSpot;    // last place any player started from

	/** Timestamp of when this game ended */
	UPROPERTY()
	float EndTime;

	/** Which actor in the game should all other actors focus on after the game is over */
	UPROPERTY()
	class AActor* EndGameFocus;

	UPROPERTY()
	TSubclassOf<class UUTLocalMessage>  DeathMessageClass;

	UPROPERTY()
	TSubclassOf<class UUTLocalMessage>  GameMessageClass;

	UPROPERTY()
	TSubclassOf<class UUTLocalMessage>  VictoryMessageClass;

	/** Name of the Scoreboard */
	UPROPERTY(Config)
	FStringClassReference ScoreboardClassName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<TSubclassOf<AUTInventory> > DefaultInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rules)
	float SpawnProtectionTime;

	UPROPERTY(Config)
	TArray<FString> MapRotation;

	/** first mutator; mutators are a linked list */
	UPROPERTY(BlueprintReadOnly, Category = Mutator)
	class AUTMutator* BaseMutator;

	virtual void InitGame( const FString& MapName, const FString& Options, FString& ErrorMessage );
	/** add a mutator by string path name */
	virtual void AddMutator(const FString& MutatorPath);
	virtual void InitGameState();
	virtual APlayerController* Login(class UPlayer* NewPlayer, const FString& Portal, const FString& Options, const TSharedPtr<class FUniqueNetId>& UniqueId, FString& ErrorMessage) override;
	virtual void Reset();
	virtual void RestartGame();
	virtual void BeginGame();
	virtual bool IsEnemy(class AController* First, class AController* Second);
	virtual void Killed(class AController* Killer, class AController* KilledPlayer, class APawn* KilledPawn, TSubclassOf<UDamageType> DamageType);
	virtual void NotifyKilled(AController* Killer, AController* Killed, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType);
	virtual void ScoreKill(AController* Killer, AController* Other);
	virtual bool CheckScore(AUTPlayerState* Scorer);
	virtual bool IsAWinner(AUTPlayerController* PC);
	virtual void SetEndGameFocus(AUTPlayerState* Winner);
	virtual void EndGame(AUTPlayerState* Winner, const FString& Reason);
	virtual void StartMatch();
	virtual void EndMatch();
	virtual void BroadcastDeathMessage(AController* Killer, AController* Other, TSubclassOf<UDamageType> DamageType);
	virtual void PlayEndOfMatchMessage();
	UFUNCTION(BlueprintCallable, Category = UTGame)
	virtual void DiscardInventory(APawn* Other, AController* Killer = NULL);

	virtual void RestartPlayer(AController* aPlayer);
	UFUNCTION(BlueprintCallable, Category = UTGame)
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;

	virtual void ChangeName(AController* Other, const FString& S, bool bNameChange);

	virtual void StartNewPlayer(APlayerController* NewPlayer);
	virtual bool ShouldSpawnAtStartSpot(AController* Player);
	virtual class AActor* FindPlayerStart( AController* Player, const FString& IncomingName = TEXT("") );
	virtual AActor* ChoosePlayerStart( AController* Player );
	virtual float RatePlayerStart(APlayerStart* P, AController* Player);

	virtual bool ReadyToStartMatch();

	virtual bool HasMatchStarted() const;
	virtual bool IsMatchInProgress() const;
	virtual bool HasMatchEnded() const;
	virtual void SetMatchState(FName NewState);

	virtual void HandleCountdownToBegin();
	virtual void CheckCountDown();

	virtual void HandleMatchHasStarted();
	virtual void HandleEnteringOvertime();
	virtual void HandleMatchInOvertime();

	virtual void ShowFinalScoreboard();
	virtual void TravelToNextMap();

	virtual void DefaultTimer();
	virtual void CheckGameTime();
	virtual AUTPlayerState* IsThereAWinner(uint32& bTied);
	virtual bool PlayerCanRestart( APlayerController* Player );

	UFUNCTION(BlueprintNativeEvent)
	void ModifyDamage(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FDamageEvent& DamageEvent, AActor* DamageCauser);

	/** used to modify, remove, and replace Actors. Return false to destroy the passed in Actor. Default implementation queries mutators.
	 * note that certain critical Actors such as PlayerControllers can't be destroyed, but we'll still call this code path to allow mutators
	 * to change properties on them
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	bool CheckRelevance(AActor* Other);

	/** changes world gravity to the specified value */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = World)
	void SetWorldGravity(float NewGravity);

	/** set or change a player's team
	 * NewTeam is a request, not a guarantee (game mode may force balanced teams, for example)
	 */
	UFUNCTION(BlueprintCallable, Category = TeamGame)
	virtual bool ChangeTeam(AController* Player, uint8 NewTeam = 255, bool bBroadcast = true);
	/** pick the best team to place this player to keep the teams as balanced as possible
	 * passed in team number is used as tiebreaker if the teams would be just as balanced either way
	 */

	virtual TSubclassOf<class AGameSession> GetGameSessionClass() const;

protected:
	/** checks whether the mutator is allowed in this gametype and doesn't conflict with any existing mutators */
	virtual bool AllowMutator(TSubclassOf<AUTMutator> MutClass);

	/**
	 * Converts a string to a bool.  If the string is empty, it will return the default.
	 **/
	inline uint32 EvalBoolOptions(FString InOpt, uint32 Default)
	{
		if (!InOpt.IsEmpty())
		{
			if (FCString::Stricmp(*InOpt,TEXT("True") )==0 
				||	FCString::Stricmp(*InOpt,*GTrue.ToString())==0
				||	FCString::Stricmp(*InOpt,*GYes.ToString())==0)
			{
				return true;
			}
			else if(FCString::Stricmp(*InOpt,TEXT("False"))==0
				||	FCString::Stricmp(*InOpt,*GFalse.ToString())==0
				||	FCString::Stricmp(*InOpt,TEXT("No"))==0
				||	FCString::Stricmp(*InOpt,*GNo.ToString())==0)
			{
				return false;
			}
			else
			{
				return FCString::Atoi(*InOpt);
			}
		}
		return Default;
	}

private:
	// hacked into ReceiveBeginPlay() so we can do mutator replacement of Actors and such
	void BeginPlayMutatorHack(FFrame& Stack, RESULT_DECL);
};





