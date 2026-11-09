using UnrealBuildTool;

public class PunchOutFrontendEditor : ModuleRules
{
	public PunchOutFrontendEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "HTTPServer", "Json", "Projects",
			"Slate", "SlateCore", "UMG", "UnrealEd", "PunchOutFrontend", "WorkspaceMenuStructure"
		});
	}
}
