using UnrealBuildTool;

public class PunchOutCore : ModuleRules
{
	public PunchOutCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "GameplayAbilities", "GameplayTags", "GameplayTasks" });
		PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "ModularGameplay" });
	}
}
