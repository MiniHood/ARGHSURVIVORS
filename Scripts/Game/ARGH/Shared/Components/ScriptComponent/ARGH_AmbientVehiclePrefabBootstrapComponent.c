// =============================================================
// ARGH_AmbientVehiclePrefabBootstrapComponent.c
//
// Attach this component to your AmbientVehicleSpawnpoint_*.et prefabs.
// It forces a refresh after loot catalog is ready (helps prefab-preview + consistent init).
// Optional: triggers catalog rebuild in WORKBENCH.
// =============================================================

class ARGH_AmbientVehiclePrefabBootstrapComponentClass : ScriptComponentClass {}

class ARGH_AmbientVehiclePrefabBootstrapComponent : ScriptComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Force refresh (runs Update selection) after init")]
	bool m_bRefreshOnInit;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "WORKBENCH: trigger catalog rebuild when prefab is loaded")]
	bool m_bRebuildCatalogInWorkbench;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "How many times to retry waiting for DL_LootSystem.vehicleDataReady")]
	int m_iRetries;

	[Attribute(defvalue: "250", uiwidget: UIWidgets.EditBox, desc: "Retry delay (ms)")]
	int m_iRetryDelayMs;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

#ifdef WORKBENCH
		if (m_bRebuildCatalogInWorkbench)
			SCR_AmbientVehicleSpawnPointComponent.TriggerCatalogRebuild();
#endif

		if (!m_bRefreshOnInit)
			return;

		if (!GetGame())
			return;

		if (m_iRetries < 1) m_iRetries = 1;
		if (m_iRetryDelayMs < 50) m_iRetryDelayMs = 50;

		GetGame().GetCallqueue().CallLater(ARGH_TryRefresh, m_iRetryDelayMs, false, m_iRetries);
	}

	void ARGH_TryRefresh(int retriesLeft)
	{
		DL_LootSystem loot = DL_LootSystem.GetInstance();
		if (!loot || !loot.vehicleDataReady)
		{
			if (retriesLeft > 0 && GetGame())
				GetGame().GetCallqueue().CallLater(ARGH_TryRefresh, m_iRetryDelayMs, false, retriesLeft - 1);
			return;
		}

		SCR_AmbientVehicleSpawnPointComponent sp = SCR_AmbientVehicleSpawnPointComponent.Cast(GetOwner().FindComponent(SCR_AmbientVehicleSpawnPointComponent));
		if (!sp)
			return;

		// Uses your public helper so we can call the protected Update safely
		sp.ARGH_RefreshNow();
	}
}
