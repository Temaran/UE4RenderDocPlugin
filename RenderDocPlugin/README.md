## Overview ##


An Unofficial Razer Hydra Plugin for the Unreal Engine 4

The plugin is designed with an event driven architecture through a delegate interface. You can access device events through Blueprintable classes provided or through C++. Main C++ support is from inheriting the HydraDelegate, through it you can extend your own custom class to support Hydra events. Additional functions in the delegate support polling for latest data.

The plugin also handles hot plugging and emits HydraPluggedIn (HydraUnplugged for the reverse), allowing you to initialize if needed when the device is ready.

[Main discussion thread](https://forums.unrealengine.com/showthread.php?3505-Razer-Hydra-Plugin)

[Unreal Engine Wiki](https://wiki.unrealengine.com/Unofficial_Hydra_Plugin)

## Quick Setup ##

 1.	Download 
 2.	Create new or choose project.
 3.	Browse to your project folder (typically found at Documents/Unreal Project/{Your Project Root})
 4.	Copy *Plugins* folder into your Project root.
 5.	Restart the Editor and open your project again.
 6.	Select Window->Plugins. Click on Installed and you should see a category called Input and a plugin called Hydra Plugin now available. It should be automatically enabled, if not, Select Enabled. The Editor will warn you to restart, click restart.
 7.	When your project has reloaded, the plugin should be enabled and ready to use.
(Optional) You can confirm it has successfully loaded by opening the Class Viewer, searching "hydra" should show you one actor class added by the plugin called HydraPluginActor.

### Input Mapping ###

 1.	For a good example start with a template project.
 2.	Use the HydraPluginController or the HydraPluginActor (NB the convenience actor needs to be placed)
 3.	Select Edit->Project Settings.
 4.	Select Engine->Input
 5.	Under Action Mappings and Axis Mappings expand the category you wish to add controller movement to. For example if you want to add Forward motion in the standard 3rd person template, click the + sign in MoveForward.
 6.	Change None to the binding you want and adjust the scale to fit. If for example you wanted this to happen when you pitch your left hydra down you would select Hydra Left Rotation Pitch with a scale of say -2.0 to have snappier controls.
 7.	Play and test your scaling adjust as needed.

(Optional) Use key and axis events in any input derived class blueprint (such as controller). Note that any events you override will cause Engine->Input mapping to stop working for that bind.


### Events through Blueprint ###

 1.	Select Window->Class Viewer.
 2.	Search for "HydraPluginActor"
 3.	Right click the actor and Create a new Blueprint e.g. "HydraPluginActorBP"
 4.	Select Graph in the upper right hand corner and right click in the graph to bring up the function search
 5.	Typing in "hydra" will narrow the events down to plugin related.
 6.	Add your desired events and linkup to your desired functions
 7.	Add the created blueprint to the scene (it's an actor subclass) and hit Play.

e.g. If you want to get the position data from your Hydras add the Event Hydra Controller Moved. Right click again in an empty space in the BP graph and add a function call to Print String, connect position to string (a conversion node will automatically be made) and drag exec (the white triangle on node) from the event to the function to connect the calls.

Compile and Play to see the position data stream as printed output after you undock your hydra.

Optionally create a blueprint subclass from HydraPlayerController and assign it in your game mode. Functionality remains the same.

See [tutorial video](https://www.youtube.com/watch?v=zRURG4Zp0Zo) for a live example.

### Events through C++ ###

#### Simple Version ####
Use, embed, or subclass HydraPluginActor or HydraPlayerController and override functions you wish to subscribe to e.g.
```virtual void HydraTriggerPressed(int32 controller) override;```

#### Extend your own Class to Receive Events through C++ ####

 1.	Include HydraDelegate.h and in your implementation
 2.	Ensure your project has "HydraPlugin" added to your PublicDependencyModuleNames in your *{Project}.build.cs*
 3.	Make your class inherit from HydraDelegate (multiple inheritence)
 4.	Copy HydraDelegate.cpp from the plugin source into your project source and right click on your *{Project}.uproject* and select *Generate Visual Studio project files*. Change top include in the HydraDelegate.cpp to "your project name.h"
 5.	Add ```HydraStartup()``` in your ```BeginPlay()``` or other initialization that is executed before the first tick.
 6.	Make your class tickable e.g. ```PrimaryActorTick.bCanEverTick = true;``` in your constructor if it isn't by default.
 7.	Add ```HydraTick(DeltaTime);``` inside ```Tick(float DeltaTime);```.
 8.	Override any of the delegate methods to receive the events.

See [tutorial video](https://www.youtube.com/watch?v=zRURG4Zp0Zo) for a live example.

## Shipping ##

When you're ready to ship

 1. As of UE4.3, your project needs code. Even if you're using blueprint only, add code to your project and compile (it doesn't need to do anything).
 2. Package your game
 3. Copy the slimmed-down *Plugins* folder found in *ShippingBuildOnly* into your packaged build *{ProjectName}* folder. E.g. if I packaged a project called *HydraTest* in my packaged directory (typically called *WindowsNoEditor*) find the *HydraTest* folder and place the Plugins folder there.
 4. Confirm its working by launching your packaged game from the *Binaries* subfolder where you placed your *Plugins* folder.

### Shipping Troubleshoot ###

You run your packaged project and you get the following errors

**Error:**

![alt text](http://i.imgur.com/IEIk7Rm.png "No Code Project Error")

Your project runtime also continues working, but your hydra does not respond.

**Fix:** This means that you have no code added to your project, as of 4.3 your project needs code to run a plugin. Add any code (e.g. new pawn that doesn't do anything extra) and compile to fix.

**Error:**

![alt text](http://i.imgur.com/j4UAp8t.png "DLL not found Error")

Also you search your log file and find 
![alt text](http://i.imgur.com/jy6nsmX.png "Log of DLL not found Error")

**Fix:** This error means the sixense dll file is missing. Copy the *Plugins* folder from ShippingBuildOnly into your *{packaged root}/{Project Name}*

## Credit ##
Plugin made by Getnamo. Point all questions to the main discussion thread.
