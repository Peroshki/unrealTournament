// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "SUWindowsDesktop.h"

#if !UE_SERVER

class SUTMenuBase : public SUWindowsDesktop
{
public:
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );
	virtual FReply OnShowServerBrowser(TSharedPtr<SComboButton> MenuButton);

protected:
	TSharedPtr<class SHorizontalBox> LeftMenuBar;
	TSharedPtr<class SHorizontalBox> RightMenuBar;
	TArray< TSharedPtr<SComboButton> > MenuButtons;

	TSharedPtr<SUWPanel> HomePanel;

	virtual void CreateDesktop();

	virtual FReply OpenPlayerSettings(TSharedPtr<SComboButton> MenuButton);
	virtual FReply OpenSystemSettings(TSharedPtr<SComboButton> MenuButton);
	virtual FReply ClearCloud(TSharedPtr<SComboButton> MenuButton);
	virtual FReply OpenTPSReport(TSharedPtr<SComboButton> MenuButton);
	virtual FReply OpenCredits(TSharedPtr<SComboButton> MenuButton);
	virtual FReply OnMenuHTTPButton(FString URL, TSharedPtr<SComboButton> MenuButton);
	virtual FReply OpenControlSettings(TSharedPtr<SComboButton> MenuButton);
	
	virtual FReply OnOnlineClick();

	virtual void BuildLeftMenuBar();
	virtual void BuildRightMenuBar();


	virtual TSharedRef<SWidget> BuildOptionsSubMenu();
	virtual TSharedRef<SWidget> BuildAboutSubMenu();
	virtual TSharedRef<SWidget> BuildOnlinePresence();

	virtual void BuildExitMenu(TSharedPtr<SComboButton> ExitButton, TSharedPtr<SVerticalBox> MenuSpace);
	
	virtual void OnOwnerLoginStatusChanged(UUTLocalPlayer* LocalPlayerOwner, ELoginStatus::Type NewStatus, const FUniqueNetId& UniqueID);

	virtual void SetInitialPanel();
	virtual FReply OnShowHomePanel();



	// The player has clicked on
	virtual FReply OnCloseClicked();
	virtual FReply OnShowServerBrowserPanel();
	virtual FReply ToggleFriendsAndFamily();
	virtual FReply OnShowStatsViewer();

	virtual FText GetBrowserButtonText() const;


private:

	// Builds out the default options for the left menu.  This includes the HOME button first and the Server Browser button last
	TSharedRef<SWidget> BuildDefaultLeftMenuBar();

	// Builds out the default options for the right menu.  Thisd includes Exit, Info and the Player Online panel
	TSharedRef<SWidget> BuildDefaultRightMenuBar();

protected:
	bool bNeedsPlayerOptions;
	bool bShowingFriends;
	
	int TickCountDown;
	virtual void OpenDelayedMenu();

};
#endif
