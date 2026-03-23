#pragma once
#include "../includes.h"
#include "../offsets/offsets.h"
#include "Player.h"

namespace Bones
{
	namespace hook
	{
		typedef const char* (__fastcall* __BoneByIndex)(uintptr_t CModel_Imp, int Index);
		extern __BoneByIndex BoneByIndex;

		extern uintptr_t hAddres;

		const char* getBone();
		inline void ToLower(char* s);
		const char* GetBoneName(CModel_Imp* model, int index);
		int FindBoneIndexByName(CModel_Imp* model, const char* targetName);
	}
}