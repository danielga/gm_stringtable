#pragma once

struct lua_State;
class CNetworkStringTable;

namespace stringtable
{

void Initialize( lua_State *state );
void Deinitialize( lua_State *state );
void Push( lua_State *state, CNetworkStringTable *stringtable );

}
