#pragma once

class CNetworkStringTableContainer;
struct lua_State;

namespace stringtablecontainer
{

extern CNetworkStringTableContainer *stcinternal;

void Initialize( lua_State *state );
void Deinitialize( lua_State *state );

}
