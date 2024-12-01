// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class Vamp_TrapTarget : TargetRules
{
	public Vamp_TrapTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "Vamp_Trap", "Core", "CoreUObject", "Engine", "InputCore", "WorldConditions", "SmartObjectsModule", "Slate" } );
	}
}
