using UnrealBuildTool;

public class PunchOutFrontend : ModuleRules
{
	public PunchOutFrontend(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "UMG", "ModelViewViewModel", "FieldNotification" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Engine", "InputCore", "Slate", "SlateCore" });
	}
}
