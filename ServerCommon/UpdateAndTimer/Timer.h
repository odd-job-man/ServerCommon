#pragma once
#include "UpdateBase.h"

namespace Timer
{
	void Reigster_UPDATE(UpdateBase* pUpdate);
	void Init();
	void Release_TimerThread();
	void Release_UpdateBase();
	void Start();
}
