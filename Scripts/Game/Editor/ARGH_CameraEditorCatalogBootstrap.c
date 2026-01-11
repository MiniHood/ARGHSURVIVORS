modded class SCR_CameraEditorComponent
{
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

#ifdef WORKBENCH
		// Defer to the central scheduler so we don't double-run.
		SCR_AmbientVehicleSpawnPointComponent.ScheduleCatalogDump();
#endif
	}
}
