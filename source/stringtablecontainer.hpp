#pragma once

class CNetworkStringTableContainer;

namespace GarrysMod
{
	namespace Lua
	{
		class ILuaBase;
	}
}

namespace stringtablecontainer
{

extern CNetworkStringTableContainer *stcinternal;

void Initialize( GarrysMod::Lua::ILuaBase *LUA );
void Deinitialize( GarrysMod::Lua::ILuaBase *LUA );

}
