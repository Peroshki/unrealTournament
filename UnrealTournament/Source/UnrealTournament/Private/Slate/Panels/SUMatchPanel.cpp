
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "../SUWindowsStyle.h"
#include "UTGameEngine.h"
#include "UTLobbyGameState.h"
#include "UTLobbyMatchInfo.h"
#include "SUMatchPanel.h"
#include "UTLobbyPC.h"

#if !UE_SERVER

void SUMatchPanel::Construct(const FArguments& InArgs)
{
	bIsFeatured = InArgs._bIsFeatured;
	MatchInfo = InArgs._MatchInfo;
	OnClicked = InArgs._OnClicked;

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SBox)						// First the overlaid box that controls everything....
		.HeightOverride(192)
		.WidthOverride(192)
		[
			SNew(SButton)
			.OnClicked(this, &SUMatchPanel::ButtonClicked)
			.ButtonStyle(SUWindowsStyle::Get(),"UWindows.Lobby.MatchButton")
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					[
						SAssignNew(MatchTitle, STextBlock)
							.Text( FString(TEXT("SOMETHING\nis\nWRONG")))
							.TextStyle(SUWindowsStyle::Get(), "UWindows.Lobby.MatchButton.TextStyle")
					]

				]
				+SVerticalBox::Slot()
				.VAlign(VAlign_Bottom)
				.HAlign(HAlign_Fill)
				.AutoHeight()
				.Padding(20.0f,0.0f,20.0f,5.0f)
				[
					SNew(SBox)
					.HeightOverride(24)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(SUWindowsStyle::Get().GetBrush("UWindows.Standard.MenuBar.Background"))
						]
						+SOverlay::Slot()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.HAlign(HAlign_Center)
							[
								SAssignNew(ActionText, STextBlock)
									.Text( this, &SUMatchPanel::GetButtonText)
									.TextStyle(SUWindowsStyle::Get(), "UWindows.Lobby.MatchButton.Action.TextStyle")
							]
						]
					]
				]
			]
		]
	];
	UpdateMatchInfo();
}

FText SUMatchPanel::GetButtonText() const
{
	if (MatchInfo.IsValid())
	{
		return MatchInfo->GetActionText();
	}

	return NSLOCTEXT("SUMatchPanel","Seutp","Initializing...");
}

void SUMatchPanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	UpdateMatchInfo();
}

void SUMatchPanel::UpdateMatchInfo()
{
	if (MatchInfo.IsValid())
	{
		SetVisibility(EVisibility::All);
		MatchTitle->SetText(MatchInfo->MatchDescription);
	}
	else
	{
		SetVisibility(EVisibility::Hidden);
	}
}

FReply SUMatchPanel::ButtonClicked()
{
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(GEngine->GetLocalPlayerFromControllerId(GWorld, 0));
	if (LocalPlayer)
	{
		AUTLobbyPC* PC = Cast<AUTLobbyPC>(LocalPlayer->PlayerController);
		if (PC)
		{
			AUTLobbyPlayerState* PS = Cast<AUTLobbyPlayerState>(PC->PlayerState);
			{
				if(PS)
				{
					PS->JoinMatch(MatchInfo.Get());
				}
			}
		}
	}
	return FReply::Handled();
}
#endif