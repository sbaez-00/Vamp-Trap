// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HudBase.generated.h"


UCLASS()
class VAMP_TRAP_API AHudBase : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnWindowsLostFocus();

	UFUNCTION(BlueprintImplementableEvent)
	void OnWindowsGainFocus();


private:
	void OnWindowFocusChanged(bool bIsFocused);
};