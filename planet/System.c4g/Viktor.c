/*-- Funktionen von Viktor --*/

#strict 2

/*
Bisher sind folgende Funktionen vorhanden:
  any   GetGraphics(int iSay, object pObj, int iOverlay)
  int   GetOverlayByIndex(int iIndex, object pObj)
  int   GetLocalCount(object pObj, int iFunc)
  any   GetLocalByIndex(int iIndex, object pObj, int iFunc)
  bool  CheckLocal(string szName, object pObj)
  bool  CheckAction(string szAction, object pObj, id idDef)
  int   FindCommand(string szName, object pObj, int iStartAt)
  int   GetSpeed(object pObj, int iPrecision)
  array CreateArray2(int iLength, int iType, int iStart)
  int   GetEnergy2(object pObj, string szSection)
  bool  ResetCon(object pObj)
  bool  MorphTo(id idMorphTo, object pObj)
  bool  MorphBack(object pObj)

  array Find_Command(int iElement, any iEqual)
  array Find_ColorDw(int iColor)
  array Find_Effect(string iName, int iPrio)
  array Find_Local(any iLocal, any iEqual)
  array Find_Team(int iTeam)
  array Find_FuncEqual(string iFunc, any iEqual)
  array Find_FuncInside(string iFunc, int iRange1, int iRange2)
  array Sort_XDistance(int iX, int iDir)
*/

global func GetGraphics(int iSay, object pObj, int iOverlay)
{
  // iSay:
  // 0 = idSrcDef
  // 1 = szGfxName
  // 2 = iOverlayMode
  // 3 = szAction
  // 4 = dwBlitMode
  // 5 = pOvelayObject
  if(!iOverlay) return(GetObjectVal("Graphics",,pObj,iSay));
  // Wenn iOverlay angegeben ist, so sollte es sich zuerst ansehen, wo das Overlay liegt.
  for(var i, iVal, iNumb = -1; iVal = GetObjectVal("GfxOverlay",,pObj,i*16); i++)
    if(iVal == iOverlay) {
      iNumb = i;
      break;
    }
  if(iNumb == -1) return(0);
  return(GetObjectVal("GfxOverlay",,pObj,iNumb*16+iSay+1));
}

global func GetOverlayByIndex(int iIndex, object pObj) { return(GetObjectVal("GfxOverlay",,pObj,iIndex*16)); }

global func GetLocalCount(object pObj, int iFunc)
{
  // iFunc:
  // 0 = Alle benannten Locals
  // 1 = Alle unbenannten Locals mit irgendeinem Wert
  if(iFunc == 0) return(GetObjectVal("LocalNamed",,pObj));
  if(iFunc == 1) {
    for(var i, iCount, iLocals = GetObjectVal("Locals",,pObj); i < iLocals; i++)
      if(GetObjectVal("Locals",,pObj,(i+1)*2) != 0) iCount++;
    return(iCount);
  }
  return(-1);
}

global func GetLocalByIndex(int iIndex, object pObj, int iFunc)
{
  // iFunc:
  // 0 = Alle benannten Locals
  // 1 = Alle unbenannten Locals mit irgendeinem Wert
  if(iFunc == 0) return(GetObjectVal("LocalNamed",,pObj,iIndex*3+1));
  if(iFunc == 1)
    for(var i, iCount, iLocals = GetObjectVal("Locals",,pObj); i < iLocals; i++)
      if(GetObjectVal("Locals",,pObj,(i+1)*2) != 0)
        if((iCount++) == iIndex) return(i);
  return(-1);
}

global func CheckLocal(string szName, object pObj)
{
  for(var i, iCount = GetLocalCount(pObj,0); i < iCount; i++)
    if(GetLocalByIndex(i,pObj,0) == szName) return(true);
  return(false);
}

global func CheckAction(string szAction, object pObj, id idDef)
{
  if(!idDef) idDef = GetID(pObj);
  return(GetActMapVal("Name",szAction,idDef) == szAction);
}

global func FindCommand(string szName, object pObj, int iStartAt)
{
  for(var i = Max(0,iStartAt), szGet; szGet = GetCommand(pObj,0,i); i++) if(szGet == szName) return(i+1);
  return(0);
}

global func GetSpeed(object pObj, int iPrecision)
{
  if(!iPrecision) iPrecision = 10;
  return(Distance(0,0,GetXDir(pObj,iPrecision),GetYDir(pObj,iPrecision)));
}

global func GetEnergy2(object pObj, string szSection) {
  return(GetObjectVal("Energy",szSection,pObj,0));
}

global func CreateArray2(int iLength, int iType, int iStart)
{
  // iType:
  // 0 = Aufsteigend sortiert
  // 1 = Absteigend sortiert
  // 2 = Zufällig
  // 3 = Gegner (iLength = Spieler)
  // 4 = Verbündete Spieler (iLength = Spieler)
  var i, iArray = [], iSave, iNum;
  if(iType == 0 || iType == 2)
    for(i = 0; i < iLength; i++) iArray[i] = (i + iStart);
  if(iType == 1)
    for(i = 0; i < iLength; i++) iArray[i] = (iLength-i-1 + iStart);
  if(iType == 2)
    for(i = 0; i < iLength; i++) {
      iNum = Random(iLength-1); if(iNum >= i) iNum + 1;
      iSave = iArray[iNum];
      iArray[iNum] = iArray[i];
      iArray[i] = iSave;
    }
  if(iType == 3 || iType == 4) {
    if(!GetPlayerName(iLength)) return([]);
    iSave = GetPlayerCount();
    for(i = 0; i < iSave; i++) {
      iNum = GetPlayerByIndex(i);
      if(iNum == iLength) continue;
      if(Hostile(iLength,iNum) == 4-iType) iArray[GetLength(iArray)] = iNum;
    }
  }

  return(iArray);
}

// Setzt die Größe des Objektes genau auf 100,000
global func ResetCon(object pObj)
{
  if(!pObj) pObj = this();
  var i, iGet, iID;
  var iNames = [], iLocal = [], iGraphic = [], iAction = [];

  iID         = GetID(pObj);
  iAction[0]  = GetAction(pObj);
  iAction[1]  = GetActionTarget(0,pObj);
  iAction[2]  = GetActionTarget(1,pObj);
  iAction[3]  = GetPhase(pObj);
  for(i = 0; iGet = GetLocalByIndex(i,pObj,0); i++) {
    iNames[i] = iGet;
    iLocal[i] = LocalN(iGet,pObj);
  }
  iGraphic[0] = GetGraphics(0,pObj);
  iGraphic[1] = GetGraphics(1,pObj);

  ObjectSetAction(pObj,"Idle",0,0,1);
  ChangeDef(TIM1,pObj);
  SetCon(100,pObj);

  ChangeDef(iID,pObj);
  ObjectSetAction(pObj,iAction[0],iAction[1],iAction[2],1);
  SetPhase(iAction[3],pObj);
  for(i = 0; iGet = iNames[i]; i++)
    LocalN(iGet,pObj) = iLocal[i];
  SetGraphics(iGraphic[1],pObj,iGraphic[0]);

  return(1);
  // Funktioniert problemlos für die meisten Clonks.
}

global func MorphTo(idMorphTo, pObj)
{
  if(!pObj) pObj = this();
  if(!idMorphTo) return(MorphBack(pObj));
  if(!GetName(0,idMorphTo) || !pObj) return(false);
  if(!GetDefCrewMember(idMorphTo)) return(false);

  var iEffect = AddEffect("IntMorphing",pObj,20,0,0,0,idMorphTo);
  return(iEffect != 0);
}

global func MorphBack(pObj)
{
  if(!pObj) pObj = this();
  if(!pObj) return(false);
  var iEffect = RemoveEffect("IntMorphing",pObj);
  return(iEffect != 0);
}

global func FxIntMorphingStart(pTarget, iEffectNumber, fTemp, idMorphTo)
{
  if(fTemp) return(0);

  EffectVar(0,pTarget,iEffectNumber) = GetID(pTarget);

  var i, i2, iLength;
  var iAction = [], iName = [], iLocal = [];

  iAction[0] = GetAction(pTarget);
  iAction[1] = GetActionTarget(0,pTarget);
  iAction[2] = GetActionTarget(1,pTarget);
  iAction[3] = GetPhase(pTarget);

  EffectVar(1,pTarget,iEffectNumber) = GetGraphics(1,pTarget);
  EffectVar(2,pTarget,iEffectNumber) = GetGraphics(0,pTarget);

  for(i = 0; i2 = GetLocalByIndex(i,pTarget,0); i++) {
    iName[i]  = i2;
    iLocal[i] = LocalN(i2,pTarget);
  }

  EffectVar(5,pTarget,iEffectNumber) = iName;

  EffectVar(6,pTarget,iEffectNumber) = GetPortrait(pTarget,0);
  EffectVar(7,pTarget,iEffectNumber) = GetPortrait(pTarget,1);

  ChangeDef(idMorphTo,pTarget);

  if(ObjectSetAction(pTarget,iAction[0],iAction[1],iAction[2],1))
    SetPhase(iAction[3] * (iAction[3] < GetActMapVal("Length",iAction[0],GetID(pTarget))),pTarget);
  else
    ObjectSetAction(pTarget,"Walk");

  iLength = GetLength(iName);
  i2 = 0;
  EffectVar(3,pTarget,iEffectNumber) = [];
  EffectVar(4,pTarget,iEffectNumber) = [];
  for(i = 0; i < iLength; i++)
    if(LocalN(iName[i],pTarget) != iLocal[i]) {
      EffectVar(3,pTarget,iEffectNumber)[i2] = iName[i];
      EffectVar(4,pTarget,iEffectNumber)[i2] = iLocal[i];
      i2++;
    }

  SetPortrait("1",pTarget,GetID(pTarget),false,false);

  return(true);
}

global func FxIntMorphingStop(pTarget, iEffectNumber, iReason, fTemp)
{
  if(fTemp) return(0);

  var i, iLength;
  var iAction = [];

  iAction[0] = GetAction(pTarget);
  iAction[1] = GetActionTarget(0,pTarget);
  iAction[2] = GetActionTarget(1,pTarget);
  iAction[3] = GetPhase(pTarget);

  ChangeDef(EffectVar(0,pTarget,iEffectNumber),pTarget);
  SetGraphics(EffectVar(1,pTarget,iEffectNumber),pTarget,EffectVar(2,pTarget,iEffectNumber));

  if(ObjectSetAction(pTarget,iAction[0],iAction[1],iAction[2],1))
    SetPhase(iAction[3] * (iAction[3] < GetActMapVal("Length",iAction[0],GetID(pTarget))),pTarget);
  else
    ObjectSetAction(pTarget,"Walk");

  iLength = GetLength(EffectVar(3,pTarget,iEffectNumber));
  for(i = 0; i < iLength; i++)
    LocalN(EffectVar(3,pTarget,iEffectNumber)[i],pTarget) = EffectVar(4,pTarget,iEffectNumber)[i];

  SetPortrait(EffectVar(6,pTarget,iEffectNumber),pTarget,EffectVar(7,pTarget,iEffectNumber),false,false);

  return(true);
}

global func FxIntMorphingEffect(szNewEffectName, pTarget, iEffectNumber, iNewEffectNumber, idMorphTo)
{
  if(GetEffect(0,pTarget,iEffectNumber,1) == szNewEffectName) {
    if(GetID(pTarget) == idMorphTo) return(-1);

    if(EffectVar(0,pTarget,iEffectNumber) == idMorphTo) {
      RemoveEffect("IntMorphing",pTarget);
      return(-1);
    }

    return(-2);
  }
  return(1);
}

global func FxIntMorphingAdd(pTarget, iEffectNumber, szNewEffectName, iNewEffectTimer, idMorphTo)
{
  var i, i2, iLength;
  var iAction = [], iName = EffectVar(5,pTarget,iEffectNumber), iLocal = [];

  iAction[0] = GetAction(pTarget);
  iAction[1] = GetActionTarget(0,pTarget);
  iAction[2] = GetActionTarget(1,pTarget);
  iAction[3] = GetPhase(pTarget);

  iLength = GetLength(iName);
  for(i = 0; i < iLength; i++) {
    if(EffectVar(3,pTarget,iEffectNumber)[i2] == iName[i]) {
      iLocal[i] = EffectVar(4,pTarget,iEffectNumber)[i2];
      i2++;
    }
    else iLocal[i] = LocalN(iName[i],pTarget);
  }

  ChangeDef(idMorphTo,pTarget);

  if(ObjectSetAction(pTarget,iAction[0],iAction[1],iAction[2],1))
    SetPhase(iAction[3] * (iAction[3] < GetActMapVal("Length",iAction[0],GetID(pTarget))),pTarget);
  else
    ObjectSetAction(pTarget,"Walk");

  EffectVar(3,pTarget,iEffectNumber) = [];
  EffectVar(4,pTarget,iEffectNumber) = [];

  i2 = 0;
  iLength = GetLength(iName);
  for(i = 0; i < iLength; i++) {
    if(CheckLocal(iName[i],pTarget)) LocalN(iName[i],pTarget) = iLocal[i];
    else {
      EffectVar(3,pTarget,iEffectNumber)[i2] = iName[i];
      EffectVar(4,pTarget,iEffectNumber)[i2] = iLocal[i];
      i2++;
    }
  }
  SetPortrait("1",pTarget,GetID(pTarget),false,false);

  return(true);
}

/*-- Find_*-Funktionen --*/

global func Find_Command(int iElement, iEqual) { return([C4FO_Func,"Find_CommandCheck",iElement,iEqual]); }
global func Find_CommandCheck(int iElement, iEqual) {
  for(var i = 0; GetCommand(0,0,i); i++)
    if(GetCommand(0,iElement,i) == iEqual) return(1);
  return(0);
}

global func Find_ColorDw(int iColor) { return([C4FO_Func,"Find_FuncEqualCheck","GetColorDw()",iColor]); }

global func Find_Effect(string iName, int iPrio) { return([C4FO_Func,"Find_EffectCheck",iName,iPrio]); }
//global func Find_EffectCheck(string iName, int iPrio) { return(GetEffectCount(iName,Object(ObjectNumber()),iPrio)); }
global func Find_EffectCheck(string iName, int iPrio) { return(GetEffect(iName,Object(ObjectNumber()),0,0,iPrio)); }

global func Find_Local(iLocal, iEqual) { return([C4FO_Func,"Find_LocalCheck",iLocal,iEqual]); }
global func Find_LocalCheck(iLocal, iEqual) { if(GetType(iLocal) == C4V_String) return(LocalN(iLocal) == iEqual); return(Local(iLocal) == iEqual); }

global func Find_Team(int iTeam) { return([C4FO_Func,"Find_FuncEqualCheck","GetPlayerTeam(GetOwner())",iTeam]); }

global func Find_FuncEqual(string iFunc, iEqual) { return([C4FO_Func,"Find_FuncEqualCheck",iFunc,iEqual]); }
global func Find_FuncEqualCheck(string iFunc, iEqual) { return(eval(iFunc) == iEqual); }

global func Find_FuncInside(string iFunc, int iRange1, int iRange2) { return([C4FO_Func,"Find_FuncInsideCheck",iFunc,iRange1,iRange2]); }
global func Find_FuncInsideCheck(string iFunc, int iRange1, int iRange2) { return(Inside(eval(iFunc),iRange1,iRange2)); }

global func Sort_XDistance(int iX, int iDir) { return([C4SO_Func,"Sort_XDistanceCheck",iX % LandscapeWidth(),iDir]); }
global func Sort_XDistanceCheck(int iX, int iDir) {
  var i;
  // Links
  if(iDir == 0) i = iX-GetX();
  // Rechts
  if(iDir == 1) i = GetX()-iX;
  // Links & Rechts
  if(iDir == 2) return(Max(GetX()-iX,iX-GetX()));
  if(i < 0) return(i + LandscapeWidth());
  return(i);
}

//global func Sort_ID(id iID) { return([C4SO_Func,"ScoreBoardCol(GetID())"]); }