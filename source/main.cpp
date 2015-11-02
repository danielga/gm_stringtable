#include <GarrysMod/Lua/Interface.h>
#include <stringtablecontainer.hpp>
#include <stringtable.hpp>

GMOD_MODULE_OPEN( )
{
	stringtablecontainer::Initialize( state );
	stringtable::Initialize( state );
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	stringtable::Deinitialize( state );
	stringtablecontainer::Deinitialize( state );
	return 0;
}
