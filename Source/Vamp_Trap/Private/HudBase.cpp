// Fill out your copyright notice in the Description page of Project Settings.


#include "HudBase.h"


void AHudBase::BeginPlay()
{
	Super::BeginPlay();

	FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &AHudBase::OnWindowFocusChanged);
}

void AHudBase::OnWindowFocusChanged(const bool bIsFocused)
{
	if (bIsFocused)
	{
		OnWindowsGainFocus();
	}
	else
	{
		OnWindowsLostFocus();
	}
}