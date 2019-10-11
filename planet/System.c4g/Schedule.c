#strict

// for now this needs to be only #strict for compatibility reasons, because old code may be evaled

global func FxIntScheduleTimer(object pObj, int iEffect)
{
	// Nur eine bestimmte Anzahl Ausführungen
	if (--EffectVar(1, pObj, iEffect) < 0) return FX_Execute_Kill;
	// Ausführen
	eval(EffectVar(0, pObj, iEffect));
}
