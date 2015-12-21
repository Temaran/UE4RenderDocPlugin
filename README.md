UE4RenderDocPlugin
==================

A plugin that adds easy access to Renderdoc inside UE4

How to use
==================

1. Make sure you are using Unreal Engine 4.8.2 or later.
   > This plugin is known to work with UE4 up to version 4.10.1

2. Copy the contents of this repository into your `<Game>/Plugins/` folder; alternatively, you can copy to the `Engine/Plugins/` if you wish to make the plugin available to all of your projects.
   > Do not forget to run UE4's `Generate Project Files` to account for these changes!

3. Download RenderDoc from http://renderdoc.org/builds
   The Stable Build v0.26 of 2015-09-25 is recommended, as the plugin requires a `renderdoc.dll` compatible with the RenderDoc v0.26 API.
   > **NOTE for advanced users:** at the time of this writing, the plugin is known to be compatible up to the 2015-12-08 nightly build of RenderDoc.

4. From within the UE4 Editor, enable both the RenderDocPlugin and the RenderDocLoaderPlugin.
   > You will need to restart the UE4 Editor for these changes to take place.

5. The first time the plugin is executed, it will attempt to find a RenderDoc installation in the system by inspecting a registry key. If you have not used a RenderDoc Installer (perhaps you downloaded a Portable version, or decided to build RenderDoc from source) then you will be prompted to locate RenderDoc manually. The plugin will remember this location for the future until it is no longer valid.
   > More specifically, it will save the RenderDoc location to the `<Game>/Saved/Config/Windows/Game.ini` configuration file.
   >
   > **NOTE for advanced users:** alternatively, you can manually point the plugin to a RenderDoc location by editing the `Engine/Config/Windows/WindowsEngine.ini` configuration file with the following:
   > ````
   > [RenderDoc]
   > BinaryPath=<path-to-your-RenderDoc-folder>
   > ````
   > This can be useful if you wish to deploy RenderDoc into a repository that is shared by a team, as the path can be relative to some Engine or Game directory.

6. After the plugin has been loaded successfully, you should have two new buttons in the top-right corner of your viewport. The left-most button will capture the next frame and launch the RenderDoc UI to inspect the captured frame. The right-most button has some configuration options for capturing.
