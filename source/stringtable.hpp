#pragma once

namespace GarrysMod
{
	namespace Lua
	{
		class ILuaBase;
	}
}

class CNetworkStringTable;

namespace stringtable
{

void Initialize( GarrysMod::Lua::ILuaBase *LUA );
void Deinitialize( GarrysMod::Lua::ILuaBase *LUA );
void Push( GarrysMod::Lua::ILuaBase *LUA, CNetworkStringTable *stringtable );

}
