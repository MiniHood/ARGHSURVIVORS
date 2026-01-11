class ARGH_DrinkWaterPlayerAction : FM_DrinkWaterPlayerAction
{
	override void ApplyHydrationEffect(IEntity characterOwner)
	{
		if (!characterOwner)
			return;

		IEntity resolvedOwner = characterOwner;
		SCR_PlayerController pc = SCR_PlayerController.Cast(characterOwner);
		if (pc)
		{
			IEntity controlled = pc.GetControlledEntity();
			if (!controlled)
				controlled = pc.GetMainEntity();

			if (controlled)
				resolvedOwner = controlled;
		}

		if (resolvedOwner && !SCR_CharacterControllerComponent.Cast(resolvedOwner.FindComponent(SCR_CharacterControllerComponent)))
		{
			IEntity parent = resolvedOwner.GetParent();
			if (parent)
				resolvedOwner = parent;
		}

		SCR_CharacterControllerComponent metabolismComponent = SCR_CharacterControllerComponent.Cast(resolvedOwner.FindComponent(SCR_CharacterControllerComponent));
		if (!metabolismComponent)
			return;

		metabolismComponent.RequestHydrationChange(m_iWaterValue);
	}
}
