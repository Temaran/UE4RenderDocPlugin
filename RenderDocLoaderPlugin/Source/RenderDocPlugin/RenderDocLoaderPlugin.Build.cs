// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    using System.IO;

    public class RenderDocLoaderPlugin : ModuleRules
    {
        public RenderDocLoaderPlugin(TargetInfo Target)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { });
            DynamicallyLoadedModuleNames.AddRange(new string[] { "LevelEditor" });

            PublicIncludePaths.AddRange(new string[] { "RenderDocLoaderPlugin/Public" });
            PrivateIncludePaths.AddRange(new string[] { "RenderDocLoaderPlugin/Private" });

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
                                                    "DesktopPlatform"
				                                });
        }
    }
}