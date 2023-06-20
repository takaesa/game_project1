#include <algorithm>
#include "debug.h"

#include "Mario.h"
#include "Game.h"

#include "Goomba.h"
#include "Coin.h"
#include "Portal.h"

#include "Collision.h"
#include "Platform.h"

#include "Brick.h"
#include "Leaf.h"
#include "MushRoom.h"
#include "QuestionBrick.h"
#include "Plain.h"
#include "FireBullet.h"
#include "Koopa.h"


void CMario::Update(DWORD dt, vector<LPGAMEOBJECT> *coObjects)
{
	vy += ay * dt;
	vx += ax * dt;

	if (abs(vx) > abs(maxVx)) vx = maxVx;

	// reset untouchable timer if untouchable time has passed
	if ( GetTickCount64() - untouchable_start > MARIO_UNTOUCHABLE_TIME) 
	{
		untouchable_start = 0;
		untouchable = 0;
	}

	if (shell != NULL && shell->GetState() == KOOPA_STATE_AWAKE)
		isCarrying = false;
	if (shell != NULL && shell->GetState() == KOOPA_STATE_CARRIED)
	{
		if (vx > 0)
		{
			shell->SetPosition(x + 16.0f, y - 1.0f);
			shell->SetDir(-1);
		}
		else if (vx < 0)
		{
			shell->SetPosition(x - 16.0f, y - 1.0f);
			shell->SetDir(1);
		}
		else
		{
			float koox, kooy;
			shell->GetPosition(koox, kooy);
			shell->SetPosition(koox, y - 1.0f);
		}
	}

	isOnPlatform = false;

	CCollision::GetInstance()->Process(this, dt, coObjects);
}

void CMario::OnNoCollision(DWORD dt)
{
	x += vx * dt;
	y += vy * dt;
}

void CMario::OnCollisionWith(LPCOLLISIONEVENT e)
{
	if (e->ny != 0 && e->obj->IsBlocking())
	{
		vy = 0;
		if (e->ny < 0) isOnPlatform = true;
	}
	else 
	if (e->nx != 0 && e->obj->IsBlocking())
	{
		vx = 0;
	}

	if (dynamic_cast<CGoomba*>(e->obj))
		OnCollisionWithGoomba(e);
	else if (dynamic_cast<CCoin*>(e->obj))
		OnCollisionWithCoin(e);
	else if (dynamic_cast<CPortal*>(e->obj))
		OnCollisionWithPortal(e);
	else if (dynamic_cast<CBrick*>(e->obj))
		OnCollisionWithBrick(e);
	else if (dynamic_cast<CLeaf*>(e->obj))
		OnCollisionWithLeaf(e);
	else if (dynamic_cast<CMushRoom*>(e->obj))
		OnCollisionWithMushRoom(e);
	else if (dynamic_cast<CQuestionBrick*>(e->obj))
		OnCollisionWithQuestionBrick(e);
	else if (dynamic_cast<CPlain*>(e->obj))
		OnCollisionWithPlain(e);
	else if (dynamic_cast<CBullet*>(e->obj))
		OnCollisionWithBullet(e);
	else if (dynamic_cast<CKoopa*>(e->obj))
		OnCollisionWithKoopa(e);
}


void CMario::OnCollisionWithGoomba(LPCOLLISIONEVENT e)
{
	CGoomba* goomba = dynamic_cast<CGoomba*>(e->obj);

	// jump on top >> kill Goomba and deflect a bit 
	if (e->ny < 0)
	{

		if (goomba->GetState() == GOOMBA_STATE_WALKING&& goomba->GetType() == 1)
		{
			goomba->SetType(0);
			goomba->Set_ay(MARIO_GRAVITY);
			vy = -MARIO_JUMP_DEFLECT_SPEED;
		}
		else if (goomba->GetState() == GOOMBA_STATE_WALKING && goomba->GetType()==0)
		{
			goomba->SetState(GOOMBA_STATE_DIE);
			vy = -MARIO_JUMP_DEFLECT_SPEED;

		}
	}
	else // hit by Goomba
	{
		if (untouchable == 0)
		{
			if (goomba->GetState() != GOOMBA_STATE_DIE)
			{
				if (level > MARIO_LEVEL_SMALL)
				{
					level = level - 1;
					StartUntouchable();
				}
				else
				{
					DebugOut(L">>> Mario DIE >>> \n");
					SetState(MARIO_STATE_DIE);
				}
			}
		}
	}
}

void CMario::OnCollisionWithKoopa(LPCOLLISIONEVENT e)
{
	CKoopa* koopa = dynamic_cast<CKoopa*>(e->obj);
	shell = koopa;
	float koox, kooy;
	koopa->GetPosition(koox, kooy);

	// Jump on top >> Koopa turns into shell and Mario deflects a bit 
	if (e->ny < 0)
	{
		if (koopa->GetState() != KOOPA_STATE_SHELL)// When Koopa is in turtle form
		{
			koopa->SetState(KOOPA_STATE_SHELL);
			vy = -MARIO_JUMP_DEFLECT_SPEED;
		}
		else if (koopa->GetState() == KOOPA_STATE_SHELL)// When Koopa is in shell form
		{
			float koox, kooy;
			koopa->GetPosition(koox, kooy);
			koopa->SetPosition(koox, kooy - 5);
			if (this->x < koox)
				koopa->SetDir(-1);
			else
				koopa->SetDir(1);

			koopa->SetState(KOOPA_STATE_SHELL_MOVING);
			vy = -MARIO_JUMP_DEFLECT_SPEED;
		}
		else if (koopa->GetState() == KOOPA_STATE_SHELL_MOVING)	// When Koopa is in shell form and moving
		{
			koopa->SetState(KOOPA_STATE_SHELL);
			vy = -MARIO_JUMP_DEFLECT_SPEED;
		}
	}
	else // Collide X with Koopa
	{
		if (hittable == 1)
		{
			if (e->nx < 0 && nx > 0)
			{
				koopa->SetStateFlipped(true);
				koopa->SetState(KOOPA_STATE_SHELL);
			}
			else if (e->nx > 0 && nx < 0)
			{
				koopa->SetStateFlipped(true);
				koopa->SetState(KOOPA_STATE_SHELL);
			}
		}
		else if (koopa->GetState() == KOOPA_STATE_SHELL || koopa->GetState() == KOOPA_STATE_SHELL + 1 || koopa->GetState() == KOOPA_STATE_CARRIED)
		{
			if (abs(ax) == abs(MARIO_ACCEL_RUN_X))
			{
				isCarrying = true;
				koopa->SetState(KOOPA_STATE_CARRIED);
				SetKickable(0);
			}
			else if (abs(ax) == abs(MARIO_ACCEL_WALK_X))
			{
				StartKickable();
				float koox, kooy;
				koopa->GetPosition(koox, kooy);
				koopa->SetPosition(koox, kooy - 5);
				if (e->nx < 0)
					koopa->SetDir(-1);
				else
					koopa->SetDir(1);

				koopa->SetState(KOOPA_STATE_SHELL_MOVING);
				SetKickable(0);
			}
		}
		else if (untouchable == 0)
		{
			if (koopa->GetState() != KOOPA_STATE_SHELL && koopa->GetState() != KOOPA_STATE_SHELL + 1
				&& kickable != 1 && koopa->GetState() != KOOPA_STATE_CARRIED)
			{
				if (level == MARIO_LEVEL_TAIL)
				{
					level = MARIO_LEVEL_BIG;
					StartUntouchable();
				}
				else if (level == MARIO_LEVEL_BIG)
				{
					level = MARIO_LEVEL_SMALL;
					StartUntouchable();
				}
				else
				{
					DebugOut(L">>> Mario DIE >>> \n");
					SetState(MARIO_STATE_DIE);
				}
			}
		}
	}
}

void CMario::OnCollisionWithCoin(LPCOLLISIONEVENT e)
{
	e->obj->Delete();
	coin++;
}

void CMario::OnCollisionWithLeaf(LPCOLLISIONEVENT e)
{
	CLeaf* leaf = dynamic_cast<CLeaf*>(e->obj);

	if (level == MARIO_LEVEL_BIG)
	{
		y = y - Push_Up_Platform * 2;
		SetLevel(MARIO_LEVEL_TAIL);
	}
	leaf->Delete();

}

void CMario::OnCollisionWithMushRoom(LPCOLLISIONEVENT e)
{
	CMushRoom* objmushroom = dynamic_cast<CMushRoom*>(e->obj);
	e->obj->Delete();
	y = y - Push_Up_Platform * 2;
	if (level == MARIO_LEVEL_SMALL)
		SetLevel(MARIO_LEVEL_BIG);
}

void CMario::OnCollisionWithPortal(LPCOLLISIONEVENT e)
{
	CPortal* p = (CPortal*)e->obj;
	CGame::GetInstance()->InitiateSwitchScene(p->GetSceneId());
}

void CMario::OnCollisionWithPlatform(LPCOLLISIONEVENT e)
{
	CPlatform* platform = dynamic_cast<CPlatform*>(e->obj);

	switch (platform->getType())
	{
	case 3:
		if (isSitting && e->ny < 0)
			platform->SetState(PLATFORM_PASSABLE);
		break;
	case 1:
		if (e->ny < 0)
		{
			y -= Push_Up_Platform;
			platform->SetState(-1);
		}
		else
		{
			platform->SetState(PLATFORM_PASSABLE);
		}
		break;
	case 2:
		DebugOut(L">>> Mario DIE >>> \n");
		SetState(MARIO_STATE_DIE);
		break;
	}
}

void CMario::OnCollisionWithQuestionBrick(LPCOLLISIONEVENT e)
{
	
	CQuestionBrick* questionbrick = dynamic_cast<CQuestionBrick*>(e->obj);
	
	if (e->ny > 0 && questionbrick->IsEmpty() == false)
	{
		if (questionbrick->GetBrickType() == 0) // coin
		{
			questionbrick->SetEmpty(true);
			float bx, by;
			questionbrick->GetPosition(bx, by);

			CCoin* coin = new CCoin(bx, by - 2*COIN_WIDTH, 1);
			LPSCENE thisscene = CGame::GetInstance()->GetCurrentScene();

			thisscene->AddObjectToScene(coin);

			coin->SetFly(true);
			questionbrick->SetPosition(bx, by);

			coin++;
			
			/*CGame::GetInstance()->GetCurrentScene()->SetCoin(coin);*/
		}
		else if (questionbrick->GetBrickType() == 1) // mushroom
		{
			questionbrick->SetEmpty(true);
			float bx, by;
			questionbrick->GetPosition(bx, by);

			CQuestionBrick* newQuestionBrick = new CQuestionBrick(bx, by, 0);
			LPSCENE thisscene = CGame::GetInstance()->GetCurrentScene();
			

			questionbrick->Delete();
			newQuestionBrick->SetPosition(bx, by);

			CMushRoom* mushroom = new CMushRoom(bx, by-32);
			newQuestionBrick->SetEmpty(true);

			thisscene->AddObjectToScene(mushroom);
			thisscene->AddObjectToScene(newQuestionBrick);
		}
		else if (questionbrick->GetBrickType() == 2) //leaf
		{
			if (level == MARIO_LEVEL_SMALL)
			{
				questionbrick->SetEmpty(true);
				float bx, by;
				questionbrick->GetPosition(bx, by);

				CQuestionBrick* newQuestionBrick = new CQuestionBrick(bx, by, 0);
				LPSCENE thisscene = CGame::GetInstance()->GetCurrentScene();


				questionbrick->Delete();
				newQuestionBrick->SetPosition(bx, by);

				CMushRoom* mushroom = new CMushRoom(bx, by - 32);
				newQuestionBrick->SetEmpty(true);

				thisscene->AddObjectToScene(mushroom);
				thisscene->AddObjectToScene(newQuestionBrick);
			}
			else {
				questionbrick->SetEmpty(true);
				float bx, by;
				questionbrick->GetPosition(bx, by);

				LPSCENE thisscene = CGame::GetInstance()->GetCurrentScene();
				CQuestionBrick* newQuesttionBrick = new CQuestionBrick(bx, by);

				questionbrick->Delete();
				newQuesttionBrick->SetPosition(bx, by);

				CLeaf* leaf = new CLeaf(bx + 16, by - 32);
				newQuesttionBrick->SetEmpty(true);

				thisscene->AddObjectToScene(leaf);
				thisscene->AddObjectToScene(newQuesttionBrick);
			}
			
		}
	}
}

void CMario::OnCollisionWithPlain(LPCOLLISIONEVENT e)
{
	CPlain* objplain = dynamic_cast<CPlain*>(e->obj);

	if (hittable == 1)
	{
		if (e->nx < 0 && nx > 0)
		{
			objplain->Delete();
		}
		else if (e->nx > 0 && nx < 0)
		{
			objplain->Delete();
		}
	}
	else if (untouchable == 0)
	{
		if (level == MARIO_LEVEL_TAIL)
			level = MARIO_LEVEL_BIG;
		else if (level == MARIO_LEVEL_BIG)
			level = MARIO_LEVEL_SMALL;
		else
			SetState(MARIO_STATE_DIE);

		StartUntouchable();
	}
}

void CMario::OnCollisionWithBullet(LPCOLLISIONEVENT e)
{
	CBullet* bullet = dynamic_cast<CBullet*>(e->obj);

	if (untouchable == 0)
	{
		if (level == MARIO_LEVEL_TAIL)
			level = MARIO_LEVEL_BIG;
		else if (level == MARIO_LEVEL_BIG)
			level = MARIO_LEVEL_SMALL;
		else
			SetState(MARIO_STATE_DIE);

		StartUntouchable();
	}

	bullet->Delete();
}


//
// Get animation ID for small Mario
//
int CMario::GetAniIdSmall()
{
	int aniId = -1;
	if (!isOnPlatform)
	{
		if (abs(ax) == MARIO_ACCEL_RUN_X)
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_SMALL_JUMP_RUN_RIGHT;
			else
				aniId = ID_ANI_MARIO_SMALL_JUMP_RUN_LEFT;
		}
		else
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_SMALL_JUMP_WALK_RIGHT;
			else
				aniId = ID_ANI_MARIO_SMALL_JUMP_WALK_LEFT;
		}
	}
	else
		if (isSitting)
		{
			if (nx > 0)
				aniId = ID_ANI_MARIO_SIT_RIGHT;
			else
				aniId = ID_ANI_MARIO_SIT_LEFT;
		}
		else
			if (vx == 0)
			{
				if (nx > 0) aniId = ID_ANI_MARIO_SMALL_IDLE_RIGHT;
				else aniId = ID_ANI_MARIO_SMALL_IDLE_LEFT;
			}
			else if (vx > 0)
			{
				if (ax < 0)
					aniId = ID_ANI_MARIO_SMALL_BRACE_RIGHT;
				else if (ax == MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_SMALL_RUNNING_RIGHT;
				else if (ax == MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_SMALL_WALKING_RIGHT;
			}
			else // vx < 0
			{
				if (ax > 0)
					aniId = ID_ANI_MARIO_SMALL_BRACE_LEFT;
				else if (ax == -MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_SMALL_RUNNING_LEFT;
				else if (ax == -MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_SMALL_WALKING_LEFT;
			}

	if (aniId == -1) aniId = ID_ANI_MARIO_SMALL_IDLE_RIGHT;

	return aniId;
}


//
// Get animdation ID for big Mario
//
int CMario::GetAniIdBig()
{
	int aniId = -1;
	if (!isOnPlatform)
	{
		if (abs(ax) == MARIO_ACCEL_RUN_X)
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_JUMP_RUN_RIGHT;
			else
				aniId = ID_ANI_MARIO_JUMP_RUN_LEFT;
		}
		else
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_JUMP_WALK_RIGHT;
			else
				aniId = ID_ANI_MARIO_JUMP_WALK_LEFT;
		}
	}
	else
		if (isSitting)
		{
			if (nx > 0)
				aniId = ID_ANI_MARIO_SIT_RIGHT;
			else
				aniId = ID_ANI_MARIO_SIT_LEFT;
		}
		else
			if (vx == 0)
			{
				if (nx > 0) aniId = ID_ANI_MARIO_IDLE_RIGHT;
				else aniId = ID_ANI_MARIO_IDLE_LEFT;
			}
			else if (vx > 0)
			{
				if (ax < 0)
					aniId = ID_ANI_MARIO_BRACE_RIGHT;
				else if (ax == MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_RUNNING_RIGHT;
				else if (ax == MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_WALKING_RIGHT;
			}
			else // vx < 0
			{
				if (ax > 0)
					aniId = ID_ANI_MARIO_BRACE_LEFT;
				else if (ax == -MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_RUNNING_LEFT;
				else if (ax == -MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_WALKING_LEFT;
			}

	if (aniId == -1) aniId = ID_ANI_MARIO_IDLE_RIGHT;

	return aniId;
}

int CMario::GetAniIdTail()
{
	int aniId = -1;
	if (!isOnPlatform)
	{
		if (abs(ax) == MARIO_ACCEL_RUN_X)
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_TAIL_JUMP_RUN_RIGHT;
			else
				aniId = ID_ANI_MARIO_TAIL_JUMP_RUN_LEFT;
		}
		else
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_TAIL_JUMP_WALK_RIGHT;
			else
				aniId = ID_ANI_MARIO_TAIL_JUMP_WALK_LEFT;
		}
	}
	else
		if (isSitting)
		{
			if (nx > 0)
				aniId = ID_ANI_MARIO_TAIL_SIT_RIGHT;
			else
				aniId = ID_ANI_MARIO_TAIL_SIT_LEFT;
		}
		else
			if (vx == 0)
			{
				if (nx > 0) aniId = ID_ANI_MARIO_TAIL_IDLE_RIGHT;
				else aniId = ID_ANI_MARIO_TAIL_IDLE_LEFT;
			}
			else if (vx > 0)
			{
				if (ax < 0)
					aniId = ID_ANI_MARIO_TAIL_BRACE_RIGHT;
				else if (ax == MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_TAIL_RUNNING_RIGHT;
				else if (ax == MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_TAIL_WALKING_RIGHT;
			}
			else // vx < 0
			{
				if (ax > 0)
					aniId = ID_ANI_MARIO_TAIL_BRACE_LEFT;
				else if (ax == -MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_TAIL_RUNNING_LEFT;
				else if (ax == -MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_TAIL_WALKING_LEFT;
			}

	if (aniId == -1) aniId = ID_ANI_MARIO_TAIL_IDLE_RIGHT;

	return aniId;
}


void CMario::Render()
{
	CAnimations* animations = CAnimations::GetInstance();
	int aniId = -1;

	if (state == MARIO_STATE_DIE)
		aniId = ID_ANI_MARIO_DIE;
	else if (level == MARIO_LEVEL_BIG)
		aniId = GetAniIdBig();
	else if (level == MARIO_LEVEL_SMALL)
		aniId = GetAniIdSmall();
	else if (level == MARIO_LEVEL_TAIL)
		aniId = GetAniIdTail();

	animations->Get(aniId)->Render(x, y);

	//RenderBoundingBox();
	
	DebugOutTitle(L"Coins: %d", coin);
}

void CMario::SetState(int state)
{
	// DIE is the end state, cannot be changed! 
	if (this->state == MARIO_STATE_DIE) return; 

	switch (state)
	{
	case MARIO_STATE_RUNNING_RIGHT:
		if (isSitting) break;
		maxVx = MARIO_RUNNING_SPEED;
		ax = MARIO_ACCEL_RUN_X;
		nx = 1;
		break;
	case MARIO_STATE_RUNNING_LEFT:
		if (isSitting) break;
		maxVx = -MARIO_RUNNING_SPEED;
		ax = -MARIO_ACCEL_RUN_X;
		nx = -1;
		break;
	case MARIO_STATE_WALKING_RIGHT:
		if (isSitting) break;
		maxVx = MARIO_WALKING_SPEED;
		ax = MARIO_ACCEL_WALK_X;
		nx = 1;
		break;
	case MARIO_STATE_WALKING_LEFT:
		if (isSitting) break;
		maxVx = -MARIO_WALKING_SPEED;
		ax = -MARIO_ACCEL_WALK_X;
		nx = -1;
		break;
	case MARIO_STATE_JUMP:
		if (isSitting) break;
		if (isOnPlatform)
		{
			if (abs(this->vx) == MARIO_RUNNING_SPEED)
				vy = -MARIO_JUMP_RUN_SPEED_Y;
			else
				vy = -MARIO_JUMP_SPEED_Y;
		}
		break;

	case MARIO_STATE_RELEASE_JUMP:
		if (vy < 0) vy += MARIO_JUMP_SPEED_Y / 2;
		break;

	case MARIO_STATE_SIT:
		if (isOnPlatform && level != MARIO_LEVEL_SMALL)
		{
			state = MARIO_STATE_IDLE;
			isSitting = true;
			vx = 0; vy = 0.0f;
			y +=MARIO_SIT_HEIGHT_ADJUST;
		}
		break;

	case MARIO_STATE_SIT_RELEASE:
		if (isSitting)
		{
			isSitting = false;
			state = MARIO_STATE_IDLE;
			y -= MARIO_SIT_HEIGHT_ADJUST;
		}
		break;

	case MARIO_STATE_IDLE:
		ax = 0.0f;
		vx = 0.0f;
		break;

	case MARIO_STATE_DIE:
		vy = -MARIO_JUMP_DEFLECT_SPEED;
		vx = 0;
		ax = 0;
		break;
	}

	CGameObject::SetState(state);
}

void CMario::GetBoundingBox(float &left, float &top, float &right, float &bottom)
{
	if (level==MARIO_LEVEL_BIG)
	{
		if (isSitting)
		{
			left = x - MARIO_BIG_SITTING_BBOX_WIDTH / 2;
			top = y - MARIO_BIG_SITTING_BBOX_HEIGHT / 2;
			right = left + MARIO_BIG_SITTING_BBOX_WIDTH;
			bottom = top + MARIO_BIG_SITTING_BBOX_HEIGHT;
		}
		else 
		{
			left = x - MARIO_BIG_BBOX_WIDTH/2;
			top = y - MARIO_BIG_BBOX_HEIGHT/2;
			right = left + MARIO_BIG_BBOX_WIDTH;
			bottom = top + MARIO_BIG_BBOX_HEIGHT;
		}
	}
	else if (level == MARIO_LEVEL_TAIL)
	{
		if (isSitting)
		{
			left = x - MARIO_TAIL_SITTING_BBOX_WIDTH / 2;
			top = y - MARIO_TAIL_SITTING_BBOX_HEIGHT / 2;
			right = left + MARIO_TAIL_SITTING_BBOX_WIDTH;
			bottom = top + MARIO_TAIL_SITTING_BBOX_HEIGHT;
		}
		else
		{
			left = x - MARIO_TAIL_BBOX_WIDTH / 2;
			top = y - MARIO_TAIL_BBOX_HEIGHT / 2;
			right = left + MARIO_TAIL_BBOX_WIDTH;
			bottom = top + MARIO_TAIL_BBOX_HEIGHT;
		}
	}
	else
	{
		left = x - MARIO_SMALL_BBOX_WIDTH/2;
		top = y - MARIO_SMALL_BBOX_HEIGHT/2;
		right = left + MARIO_SMALL_BBOX_WIDTH;
		bottom = top + MARIO_SMALL_BBOX_HEIGHT;
	}

}

void CMario::SetLevel(int l)
{
	// Adjust position to avoid falling off platform
	if (this->level == MARIO_LEVEL_SMALL)
	{
		y -= 10;
	}
	else if (this->level == MARIO_LEVEL_BIG)
	{
		y -= 10;
	}else if (this->level == MARIO_LEVEL_TAIL)
	{
		y -= 10;
	}
	level = l;
}

