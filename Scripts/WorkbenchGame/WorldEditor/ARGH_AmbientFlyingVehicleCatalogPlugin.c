#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "ARGH Ambient Flying Vehicle Catalog", wbModules: {"WorldEditor"}, awesomeFontCode: 0xf072)]
class ARGH_AmbientFlyingVehicleCatalogPlugin : WorkbenchPlugin
{
	override void Run()
	{
		Print("[ARGH_FLYING_CATALOG] Workbench plugin triggered - resetting catalog state and forcing rebuild");
		ARGH_SCR_AmbientFlyingVehicleSpawnPointComponent.ResetFlyingCatalogState();
		ARGH_SCR_AmbientFlyingVehicleSpawnPointComponent.ForceRebuildFlyingCatalog();
	}
	
	[ButtonAttribute("Rebuild Flying Vehicle Catalog")]
	void RebuildCatalog()
	{
		Print("[ARGH_FLYING_CATALOG] Manual rebuild triggered from Workbench plugin");
		ARGH_SCR_AmbientFlyingVehicleSpawnPointComponent.ForceRebuildFlyingCatalog();
	}
}
#endif
