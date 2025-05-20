// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RandomGod : ModuleRules
{
	public RandomGod(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] 
			{   "Core", 
				"CoreOnline",
				"CoreUObject", 
				"Engine",
				"InputCore",
				"EnhancedInput",
				"BattleGameplayLibrary",
				"NetCore",
				"GameplayTags",
				"ModularGameplay",
				"ModularGameplayActors",
				"GameFeatures",
				"CommonUser",
				"GameplayAbilities",
				"CommonUI",
				"UMG",
				"DeveloperSettings",
			});
		
		SetupIrisSupport(Target);
	}
}
