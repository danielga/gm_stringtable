#include <GarrysMod/Lua/Interface.h>
#include <stringtablecontainer.hpp>
#include <stringtable.hpp>

GMOD_MODULE_OPEN( )
{
	stringtablecontainer::Initialize( LUA );
	stringtable::Initialize( LUA );
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	stringtable::Deinitialize( LUA );
	stringtablecontainer::Deinitialize( LUA );
	return 0;
}
