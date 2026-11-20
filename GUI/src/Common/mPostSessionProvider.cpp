#include "mPostSessionProvider.h"

namespace MechCommon
{
	IPostSessionParticipant*& postSessionParticipantRef()
	{
		static IPostSessionParticipant* inst = nullptr;
		return inst;
	}
}
