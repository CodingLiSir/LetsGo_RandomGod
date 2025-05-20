// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RandomGod : ModuleRules
{
	public RandomGod(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "CommonLoadingScreen" });
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
				"CommonLoadingScreen",
				"GameplayAbilities",
				"CommonUI",
				"UMG",
				"DeveloperSettings",
			});
		
		SetupIrisSupport(Target);
	}
}
