namespace UnrealBuildTool.Rules
{
    using System.IO;

    public class RenderDocPlugin : ModuleRules
    {
        public RenderDocPlugin(TargetInfo Target)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { });
            DynamicallyLoadedModuleNames.AddRange(new string[] { "LevelEditor" });

            PublicIncludePaths.AddRange(new string[] { "RenderDocPlugin/Public" });
            PrivateIncludePaths.AddRange(new string[] { "RenderDocPlugin/Private" });

            PublicDependencyModuleNames.AddRange(new string[]
				                                {
					                                "Core",
					                                "CoreUObject",
                                                    "Engine",
                                                    "InputCore",
                                                    "Slate", 
                                                    "SlateCore",
                                                    "EditorStyle",
                                                    "LevelEditor",
                                                    "UnrealEd",
                                                    "RenderCore"
				                                });
        }
    }
}