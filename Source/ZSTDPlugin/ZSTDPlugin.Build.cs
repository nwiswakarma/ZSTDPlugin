/*
 ZSTDPlugin 0.0.1
 -----
 
*/
using System.IO;
using System.Collections;
using UnrealBuildTool;

public class ZSTDPlugin: ModuleRules
{
    public ZSTDPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        Definitions.Add("ZSTD_STATIC_LINKING_ONLY");

		PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine"
            });

		PrivateDependencyModuleNames.AddRange(new string[] { });

        string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty");

        // -- ZSTD include and lib path

        string ZSTDPath = Path.Combine(ThirdPartyPath, "ZSTD");
        string ZSTDInclude = Path.Combine(ZSTDPath, "include");
        string ZSTDLib = Path.Combine(ZSTDPath, "lib");

		PublicIncludePaths.Add(Path.GetFullPath(ZSTDInclude));
        PublicLibraryPaths.Add(Path.GetFullPath(ZSTDLib));

        PublicAdditionalLibraries.Add("ZSTD.lib");
	}
}
