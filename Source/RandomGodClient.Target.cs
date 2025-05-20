// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RandomGodClientTarget : TargetRules
{
	public RandomGodClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("RandomGod");
	}
}
