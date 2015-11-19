#include "Weapon_RocketLauncher.h"
#include "../Raven_Bot.h"
#include "misc/Cgdi.h"
#include "../Raven_Game.h"
#include "../Raven_Map.h"
#include "../lua/Raven_Scriptor.h"
#include "fuzzy/FuzzyOperators.h"


//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------
RocketLauncher::RocketLauncher(Raven_Bot*   owner):

                      Raven_Weapon(type_rocket_launcher,
                                   script->GetInt("RocketLauncher_DefaultRounds"),
                                   script->GetInt("RocketLauncher_MaxRoundsCarried"),
                                   script->GetDouble("RocketLauncher_FiringFreq"),
                                   script->GetDouble("RocketLauncher_IdealRange"),
                                   script->GetDouble("Rocket_MaxSpeed"),
                                   owner)
{
    //setup the vertex buffer
  const int NumWeaponVerts = 8;
  const Vector2D weapon[NumWeaponVerts] = {Vector2D(0, -3),
                                           Vector2D(6, -3),
                                           Vector2D(6, -1),
                                           Vector2D(15, -1),
                                           Vector2D(15, 1),
                                           Vector2D(6, 1),
                                           Vector2D(6, 3),
                                           Vector2D(0, 3)
                                           };
  for (int vtx=0; vtx<NumWeaponVerts; ++vtx)
  {
    m_vecWeaponVB.push_back(weapon[vtx]);
  }

  //setup the fuzzy module
  InitializeFuzzyModule();

}


//------------------------------ ShootAt --------------------------------------
//-----------------------------------------------------------------------------
inline void RocketLauncher::ShootAt(Vector2D pos)
{ 
  if (NumRoundsRemaining() > 0 && isReadyForNextShot())
  {
    //fire off a rocket!
    m_pOwner->GetWorld()->AddRocket(m_pOwner, pos);

    m_iNumRoundsLeft--;

    UpdateTimeWeaponIsNextAvailable();

    //add a trigger to the game so that the other bots can hear this shot
    //(provided they are within range)
    m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("RocketLauncher_SoundRange"));
  }
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------
double RocketLauncher::GetDesirability(double DistToTarget)
{
  if (m_iNumRoundsLeft == 0)
  {
    m_dLastDesirabilityScore = 0;
  }
  else
  {
    //fuzzify distance and amount of ammo
    m_FuzzyModule.Fuzzify("DistToTarget", DistToTarget);
    m_FuzzyModule.Fuzzify("AmmoStatus", (double)m_iNumRoundsLeft);

    m_dLastDesirabilityScore = m_FuzzyModule.DeFuzzify("Desirability", FuzzyModule::max_av);
  }

  return m_dLastDesirabilityScore;
}

//-------------------------  InitializeFuzzyModule ----------------------------
//
//  set up some fuzzy variables and rules
//-----------------------------------------------------------------------------
void RocketLauncher::InitializeFuzzyModule()
{
  FuzzyVariable& DistToTarget = m_FuzzyModule.CreateFLV("DistToTarget");

  FzSet& Target_VeryClose = DistToTarget.AddLeftShoulderSet("Target_VeryClose",0,25,50);
  FzSet& Target_Close = DistToTarget.AddTriangularSet("Target_Close",25,50,150);
  FzSet& Target_Medium = DistToTarget.AddTriangularSet("Target_Medium",50,150,300);
  FzSet& Target_Far = DistToTarget.AddTriangularSet("Target_Far",150,300,600);
  FzSet& Target_VeryFar = DistToTarget.AddRightShoulderSet("Target_VeryFar", 300, 600, 1000);

  FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability"); 
  FzSet& VeryDesirable = Desirability.AddRightShoulderSet("VeryDesirable", 60, 85, 100);
  FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 30, 60, 85);
  FzSet& MiddleDesirable = Desirability.AddTriangularSet("MiddleDesirable", 10, 30, 60);
  FzSet& UnDesirable = Desirability.AddLeftShoulderSet("Undesirable", 0, 10, 30);
  FzSet& NonDesirable = Desirability.AddSingletonSet("Nondesirable", 0, 0, 0);


  FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");
  FzSet& Ammo_Full = AmmoStatus.AddRightShoulderSet("Ammo_Full", 60, 90, 100);
  FzSet& Ammo_Loads = AmmoStatus.AddTriangularSet("Ammo_Loads", 30, 60, 90);
  FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 10, 30, 60);
  FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Okay", 5, 10, 30);
  FzSet& Ammo_VeryLow = AmmoStatus.AddLeftShoulderSet("Ammo_Low", 0, 5, 10);

  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Full), NonDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Loads), NonDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Okay), NonDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Low), NonDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_VeryLow), NonDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Full), MiddleDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads), MiddleDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay), UnDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low), UnDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_VeryLow), NonDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Full), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay), MiddleDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low), MiddleDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_VeryLow), UnDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Full), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay), MiddleDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low), UnDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_VeryLow), NonDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Full), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Loads), MiddleDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Okay), UnDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Low), NonDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_VeryLow), NonDesirable);

}


//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void RocketLauncher::Render()
{
    m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
                                   m_pOwner->Pos(),
                                   m_pOwner->Facing(),
                                   m_pOwner->Facing().Perp(),
                                   m_pOwner->Scale());

  gdi->RedPen();

  gdi->ClosedShape(m_vecWeaponVBTrans);
}