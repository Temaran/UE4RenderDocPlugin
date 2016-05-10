UE4RenderDocPlugin
==================

A plugin that adds easy access to Renderdoc inside Unreal Engine 4.  
Currently, this plugin only runs on Windows systems.



How to Use
----------

1. Make sure you are using Unreal Engine 4.10.4 or later.  
   This plugin is also known to work with all UE4 versions from 4.8.2 up to 4.11 (preview 6).  
   There are commit tags in the repository for older versions of the plugin that suit former versions of UE4.

2. Copy the contents of this repository into your `<Game>/Plugins/` folder.  
   Alternatively, you may copy it to the `UE4/Engine/Plugins/` folder, thus making the plugin available to all of your projects.

3. In order to build the plugin, make sure to run UE4's `Generate Project Files` to register the plugin source code with Unreal Build Tool.

4. Download and install RenderDoc from http://renderdoc.org/builds  
   The stable build v0.29 of 2016-05-08 is recommended.

5. From within the UE4 Editor, enable both the RenderDocPlugin and the RenderDocLoaderPlugin, as shown below. Note that you will need to restart the UE4 Editor for these changes to take place.  
   ![](doc/img/howto-plugin_menu.jpg)   ![](doc/img/howto-enable.jpg)

6. The first time the plugin is executed, it will attempt to automatically find a RenderDoc installation.  
   If unable to locate one, you will be prompted to locate RenderDoc manually through a dialog window.  
   The plugin will remember the RenderDoc location until it is no longer valid.

7. After the plugin has been loaded successfully, you should have two new buttons in the top-right corner of your Level Editor viewport.  
   The left-most button (see below) will capture the next frame and launch the RenderDoc UI to inspect the frame.  
   The right-most button has some configuration options.  
   ![](doc/img/howto-capture.jpg)

8. Alternatively, the console command `RenderDoc.CaptureFrame` can also be used for capturing a frame. This is particularly useful when in PIE (Play-in-Editor) mode or when in Game mode, as the Level Editor viewport UI is omitted during gameplay.



For Advanced Users
------------------

* This version of the plugin relies upon a `renderdoc.dll` compatible with the RenderDoc v0.26 API.  
  Other RenderDoc builds that retain API compatibility with RenderDoc v0.26 should also work with this version of the plugin.

* The very first time the plugin runs, a valid RenderDoc installation will be inferred by inspecting the following Windows registry key:  
  `HKEY_LOCAL_MACHINE\SOFTWARE\Classes\RenderDoc.RDCCapture.1\DefaultIcon\`  
If RenderDoc can not be located in this registry key (perhaps because you wish to use a portable version of RenderDoc, or decided to build RenderDoc from source), you will be asked to locate `renderdocui.exe` manually through a dialog window.  
The plugin will then keep track of this RenderDoc location by adding an entry to the following UE4 configuration file:  
  `<Game>/Saved/Config/Windows/Game.ini`

* You may also explicitly direct the plugin to a RenderDoc location by editing the following configuration file  
  `Engine/Config/BaseGame.ini`  
  and adding the following entry to it:  
  ````ini
  [RenderDoc]
  BinaryPath=<path-to-your-RenderDoc-folder>
  GreetingHasBeenShown=True
  ````
  This method can be very useful if you wish to deploy RenderDoc into repositories that are shared by entire teams, as the RenderDoc path can be relative to some Game or Engine directory.

* You may force plugin compilation by adding the following to the `.uplugin` files:
  ```json
  "EnabledByDefault" : true,
  ```
  Note that this will not only force-build the plugin, but will also keep the plugin activated at all times.  
  You can then selectively disable the plugin by inserting the following into your `<Game>.uproject` file:
  ```json
  {
    "Name": "RenderDocLoaderPlugin",
    "Enabled": false
  },
  {
    "Name": "RenderDocPlugin",
    "Enabled": false
  },
  ```
