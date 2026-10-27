// minGameplayFramework game module entry.
// The host links this DLL with the entry symbol forced into the import table, so the
// module is loaded (and its reflection registrations run) before any engine call.

#include "Runtime/Core/Module/MinEngineGameModuleEntry.h"

#include "GameInstance/MinGameInstance.hpp"
#include "MinGameplayFrameworkAPI.hpp"

MINENGINE_IMPLEMENT_GAME_MODULE_ENTRY(MINGAMEPLAYFRAMEWORK_API)
