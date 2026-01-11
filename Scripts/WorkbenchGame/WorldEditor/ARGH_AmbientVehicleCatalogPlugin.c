#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "ARGH Ambient Vehicle Catalog", wbModules: {"WorldEditor"}, awesomeFontCode: 0xf1b9)]
class ARGH_AmbientVehicleCatalogPlugin : WorkbenchPlugin
{
	override void Run()
	{
		Print("[ARGH_VEHICLE_CATALOG] Workbench plugin triggered - resetting catalog state and forcing rebuild");
		SCR_AmbientVehicleSpawnPointComponent.ResetCatalogState();
		SCR_AmbientVehicleSpawnPointComponent.ForceRebuildCatalog();
	}
	
	[ButtonAttribute("Rebuild Vehicle Catalog")]
	void RebuildCatalog()
	{
		Print("[ARGH_VEHICLE_CATALOG] Manual rebuild triggered from Workbench plugin");
		SCR_AmbientVehicleSpawnPointComponent.ForceRebuildCatalog();
	}
}
#endif
