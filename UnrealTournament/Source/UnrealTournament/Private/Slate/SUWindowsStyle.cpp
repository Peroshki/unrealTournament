// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "Slate/SlateGameResources.h"
#include "SUWindowsStyle.h"

TSharedPtr<FSlateStyleSet> SUWindowsStyle::UWindowsStyleInstance = NULL;

void SUWindowsStyle::Initialize()
{
	if (!UWindowsStyleInstance.IsValid())
	{
		UWindowsStyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle( *UWindowsStyleInstance);
	}
}

void SUWindowsStyle::Shutdown()
{
}

FName SUWindowsStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("UWindowsStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".ttf"), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( FPaths::GameContentDir() / "RestrictedAssets/Slate"/ RelativePath + TEXT(".otf"), __VA_ARGS__ )


TSharedRef<FSlateStyleSet> SUWindowsStyle::Create()
{
	TSharedRef<FSlateStyleSet> StyleRef = MakeShareable(new FSlateStyleSet(SUWindowsStyle::GetStyleSetName()));

	StyleRef->SetContentRoot(FPaths::GameContentDir() / TEXT("RestrictedAssets/Slate"));
	StyleRef->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	FSlateStyleSet& Style = StyleRef.Get();

	{ // Menu Bar Background

		Style.Set("UWindows.Standard.MenuBar.Background", new BOX_BRUSH("UWindows.Standard.MenuBar.Background", FMargin(8.0f / 32.0f, 8.0f / 32.0f, 8.0f / 32.0f, 8.0f / 32.0f)));
	
		Style.Set("UWindows.Standard.MenuButton", FButtonStyle()
			.SetNormal ( FSlateNoResource( FVector2D( 128.0f, 128.0f ) ))
			.SetHovered( BOX_BRUSH("UWindows.Standard.MenuButton.Hovered", FMargin(8.0f / 64.0f, 8.0f / 32.0f, 8.0f / 64.0f, 8.0f / 32.0f)))
			.SetPressed( BOX_BRUSH("UWindows.Standard.MenuButton.Pressed", FMargin(8.0f / 64.0f, 8.0f / 32.0f, 8.0f / 64.0f, 8.0f / 32.0f)))
			.SetDisabled(BOX_BRUSH("UWindows.Standard.MenuButton.Disabled", FMargin(8.0f / 64.0f, 8.0f / 32.0f, 8.0f / 64.0f, 8.0f / 32.0f)))
			);
	}

	{  // Standard Button

		Style.Set("UWindows.Standard.Button", FButtonStyle()
			.SetNormal ( BOX_BRUSH("UWindows.Standard.Button.Normal", FMargin(8.0f / 64.0f, 8.0f / 32.0f, 8.0f / 64.0f, 8.0f / 32.0f)))
			.SetHovered( BOX_BRUSH("UWindows.Standard.Button.Hovered", FMargin(8.0f / 64.0f, 8.0f / 32.0f, 8.0f / 64.0f, 8.0f / 32.0f)))
			.SetPressed( BOX_BRUSH("UWindows.Standard.Button.Pressed", FMargin(8.0f / 64.0f, 8.0f / 32.0f, 8.0f / 64.0f, 8.0f / 32.0f)))
			.SetDisabled(BOX_BRUSH("UWindows.Standard.Button.Disabled", FMargin(8.0f / 64.0f, 8.0f / 32.0f, 8.0f / 64.0f, 8.0f / 32.0f)))
			);
	}

	return StyleRef;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT


void SUWindowsStyle::Reload()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& SUWindowsStyle::Get()
{
	return * UWindowsStyleInstance;
}