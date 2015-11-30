#include "Raven_TargetingSystem.h"
#include "Raven_Bot.h"
#include "Raven_SensoryMemory.h"



//-------------------------------- ctor ---------------------------------------
//-----------------------------------------------------------------------------
Raven_TargetingSystem::Raven_TargetingSystem(Raven_Bot* owner):m_pOwner(owner),
                                                               m_pCurrentTarget(0)
{}



//----------------------------- Update ----------------------------------------

//-----------------------------------------------------------------------------
void Raven_TargetingSystem::Update()
{
  double ClosestDistSoFar = MaxDouble;
  m_pCurrentTarget       = 0;

  //grab a list of all the opponents the owner can sense
  std::list<Raven_Bot*> SensedBots;
  SensedBots = m_pOwner->GetSensoryMem()->GetListOfRecentlySensedOpponents();
  
  std::list<Raven_Bot*>::const_iterator curBot = SensedBots.begin();
  for (curBot; curBot != SensedBots.end(); ++curBot)
  {
    //make sure the bot is alive and that it is not the owner
    if ((*curBot)->isAlive() && (*curBot != m_pOwner) )
    {
		if (m_pOwner->GetTeam() != (*curBot)->GetTeam())
		{
			double dist = Vec2DDistanceSq((*curBot)->Pos(), m_pOwner->Pos());

			if (dist < ClosestDistSoFar)
			{
				ClosestDistSoFar = dist;
				m_pCurrentTarget = *curBot;
			}
		}
    }
  } 
  //permet de réduire les tirs alliés
  if (allyBeforeTarget()){
	  m_pCurrentTarget = 0;
  }
}

bool Raven_TargetingSystem::allyBeforeTarget(){
	if (!m_pCurrentTarget){
		return false;
	}
	
	//grab a list of all the opponents the owner can sense
	std::list<Raven_Bot*> SensedBots;
	SensedBots = m_pOwner->GetSensoryMem()->GetListOfRecentlySensedOpponents();

	std::list<Raven_Bot*>::const_iterator curBot = SensedBots.begin();
	for (curBot; curBot != SensedBots.end(); ++curBot)
	{
		//make sure the bot is alive and that it is not the owner
		if ((*curBot)->isAlive() && (*curBot != m_pOwner))
		{
			if (m_pOwner->GetTeam() == (*curBot)->GetTeam())
			{
				if (fonctionAffine(m_pOwner->Pos(), m_pCurrentTarget->Pos(), (*curBot)->Pos()))
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool Raven_TargetingSystem::fonctionAffine(Vector2D p1, Vector2D p2, Vector2D p3){
	if (p1 == p2) return false;
	double deltax = p1.x - p2.x;
	double fp3x = ((p1.y - p2.y) / deltax) * p3.x + ((p1.x * p2.y - p2.x * p1.y) / deltax);
	return (p3.y <= (fp3x + 3) && p3.y >= fp3x - 3);
}


bool Raven_TargetingSystem::isTargetWithinFOV()const
{
  return m_pOwner->GetSensoryMem()->isOpponentWithinFOV(m_pCurrentTarget);
}

bool Raven_TargetingSystem::isTargetShootable()const
{
  return m_pOwner->GetSensoryMem()->isOpponentShootable(m_pCurrentTarget);
}

Vector2D Raven_TargetingSystem::GetLastRecordedPosition()const
{
  return m_pOwner->GetSensoryMem()->GetLastRecordedPositionOfOpponent(m_pCurrentTarget);
}

double Raven_TargetingSystem::GetTimeTargetHasBeenVisible()const
{
  return m_pOwner->GetSensoryMem()->GetTimeOpponentHasBeenVisible(m_pCurrentTarget);
}

double Raven_TargetingSystem::GetTimeTargetHasBeenOutOfView()const
{
  return m_pOwner->GetSensoryMem()->GetTimeOpponentHasBeenOutOfView(m_pCurrentTarget);
}
