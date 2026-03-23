#include "hooks.h"

namespace Bones
{
	namespace hook
	{
		extern uintptr_t hAddres = (uintptr_t)FindSignature((void*)offset::clientDLL,
			"85 D2 78 ? 3B 91 ? ? ? ? 7D ? 48 8B 81 ? ? ? ? 48 63 D2 ? ? ? ? 48 8D 05 ? ? ? ? 48 85 C9 48 0F 45 C1 C3 48 8D 05");

		extern __BoneByIndex BoneByIndex = (__BoneByIndex)hAddres;

		const char* getBone()
		{
			PlayerPawn* p = (PlayerPawn*)offset::CPlayerPawn;

			SceneNode* sn = (SceneNode*)p->GetNode();

			SkeletoneInstance* sk = (SkeletoneInstance*)sn->GetSkeletoneInstance();

			CModel_Imp* mi = (CModel_Imp*)(*(uintptr_t*)(sk->GetCModel_Imp()));

			__BoneByIndex bone = (__BoneByIndex)hAddres;
			const char* b = bone((uintptr_t)mi, 7);
			
			return b;
		}

		const char* GetBoneName(CModel_Imp* model, int index)
		{
			if (!model || !Bones::hook::BoneByIndex)
				return nullptr;

			return Bones::hook::BoneByIndex((uintptr_t)model, index);
		}

		inline void ToLower(char* s)
		{
			for (; *s; ++s)
				*s = (char)tolower(*s);
		}


		int FindBoneIndexByName(CModel_Imp* model, const char* targetName)
		{
			if (!model || !targetName)
				return -1;

			char lowerTarget[64];
			strcpy_s(lowerTarget, targetName);
			ToLower(lowerTarget);

			for (int i = 0; i < 128; i++)
			{
				const char* boneName = GetBoneName(model, i);
				if (!boneName)
					continue;

				char lowerBone[128];
				strcpy_s(lowerBone, boneName);
				ToLower(lowerBone);

				if (strstr(lowerBone, lowerTarget))
					return i;
			}

			return -1;
		}

	}
}