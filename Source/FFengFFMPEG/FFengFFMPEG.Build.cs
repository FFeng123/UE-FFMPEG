// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class FFengFFMPEG : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }

    private string UProjectPath
    {
        get { return Directory.GetParent(ModulePath).Parent.FullName; }
    }

    private void CopyToBinaries(string FilePath)
    {
        string binariesDir = Path.Combine(UProjectPath, "Binaries", Target.Platform.ToString());
        string filename = Path.GetFileName(FilePath);

        System.Console.WriteLine("Writing file " + FilePath + " to " + binariesDir);

        if (!Directory.Exists(binariesDir))
        {
            Directory.CreateDirectory(binariesDir);
        }

        if (!File.Exists(Path.Combine(binariesDir, filename)))
        {
            File.Copy(FilePath, Path.Combine(binariesDir, filename), true);
        }
    }

    public bool LoadFFmpeg(ReadOnlyTargetRules Target)
    {
        bool isLibrarySupported = false;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            isLibrarySupported = true;

            string LibrariesPath = Path.Combine(ThirdPartyPath, "ffmpeg", "libs");
            string BinariesPath = Path.Combine(ThirdPartyPath, "ffmpeg", "bin");

            PublicRuntimeLibraryPaths.Add(BinariesPath);


            System.Console.WriteLine("... LibrariesPath ---->" + LibrariesPath);


            PublicAdditionalLibraries.AddRange(
                new string[]
                {
                    Path.Combine(LibrariesPath, "avcodec.lib"),
                    Path.Combine(LibrariesPath, "avdevice.lib"),
                    Path.Combine(LibrariesPath, "avfilter.lib"),
                    Path.Combine(LibrariesPath, "avformat.lib"),
                    //Path.Combine(LibrariesPath, "AviSynth.lib"),
                    //Path.Combine(LibrariesPath, "avresample.lib"),
                    Path.Combine(LibrariesPath, "avutil.lib"),
                    //Path.Combine(LibrariesPath, "libmp3lame.lib"),
                    //Path.Combine(LibrariesPath, "libx264.lib"),
                    Path.Combine(LibrariesPath, "postproc.lib"),
                    Path.Combine(LibrariesPath, "swresample.lib"),
                    Path.Combine(LibrariesPath, "swscale.lib"),
                    //Path.Combine(LibrariesPath, "wavpackdll.lib"),
                }
            );


            string[] dllNames =
            {
                "avcodec-61.dll",
                "avdevice-61.dll",
                "avfilter-10.dll",
                "avformat-61.dll",
                //"AviSynth.dll",
                //"avresample-4.dll",
                "avutil-59.dll",
                //"libmp3lame.dll",
                //"libx264-157.dll",
                "postproc-58.dll",
                "swresample-5.dll",
                "swscale-8.dll",
                //"wavpackdll.dll",
			};

            foreach (string dllName in dllNames)
            {
                PublicDelayLoadDLLs.Add(dllName);
                RuntimeDependencies.Add(Path.Combine(BinariesPath, dllName), StagedFileType.NonUFS);
                CopyToBinaries(Path.Combine(BinariesPath, dllName));
            }

            if (isLibrarySupported)
            {
                isLibrarySupported = true;
                PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "ffmpeg", "include"));
            }

        }



        return isLibrarySupported;
    }

    public bool LoadRTMP()
    {
        string SrcPath = Path.Combine(ThirdPartyPath, "rtmp", "src");
        PrivateIncludePaths.AddRange(
            new string[]{
                SrcPath,
                Path.Combine(SrcPath, "3rdpart")
            }
            );
        string libType = (Target.Configuration == UnrealTargetConfiguration.Debug ? "Debug" : "Release");
        string LibrariesPath = Path.Combine(ThirdPartyPath, "rtmp", "VS2017", "bin", "x64", libType);
        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "rtmp-server.lib"));

        return true;
    }

    public FFengFFMPEG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",

            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Engine",
                "CoreUObject",
                "Slate",
                "SlateCore",
                "Projects",
                "MovieSceneCapture",
                "RHI",
                "RenderCore",
                "AudioMixer",
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );

        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }
        LoadFFmpeg(Target);
        LoadRTMP();
    }
}
