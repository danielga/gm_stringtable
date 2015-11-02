#include <stringtablecontainer.hpp>
#include <stringtable.hpp>
#include <GarrysMod/Lua/Interface.h>
#include <hackednetworkstringtable.h>
#include <interfaces.hpp>
#include <cstdint>

namespace stringtablecontainer
{

static SourceSDK::FactoryLoader engine_loader( "engine", false, IS_SERVERSIDE );

#if defined STRINGTABLE_SERVER

static const char *networkstringtable_interface = INTERFACENAME_NETWORKSTRINGTABLESERVER;

#elif defined STRINGTABLE_CLIENT

static const char *networkstringtable_interface = INTERFACENAME_NETWORKSTRINGTABLECLIENT;

#endif

CNetworkStringTableContainer *stcinternal = nullptr;

static const char *table_name = "stringtable";

LUA_FUNCTION_STATIC( Find )
{
	stringtable::Push( state, static_cast<CNetworkStringTable *>(
		stcinternal->FindTable( LUA->CheckString( 1 ) )
	) );
	return 1;
}

LUA_FUNCTION_STATIC( Get )
{
	stringtable::Push( state, static_cast<CNetworkStringTable *>(
		stcinternal->GetTable( static_cast<int32_t>( LUA->CheckNumber( 1 ) ) )
	) );
	return 1;
}

LUA_FUNCTION_STATIC( GetCount )
{
	LUA->PushNumber( stcinternal->GetNumTables( ) );
	return 1;
}

void Initialize( lua_State *state )
{
	stcinternal = engine_loader.GetInterface<CNetworkStringTableContainer>( networkstringtable_interface );
	if( stcinternal == nullptr )
		LUA->ThrowError( "unable to initialize INetworkStringTableContainer" );

	LUA->CreateTable( );

	LUA->PushString( "stringtable 1.0.0" );
	LUA->SetField( -2, "Version" );

	// version num follows LuaJIT style, xxyyzz
	LUA->PushNumber( 10000 );
	LUA->SetField( -2, "VersionNum" );

	LUA->PushCFunction( Find );
	LUA->SetField( -2, "Find" );

	LUA->PushCFunction( Get );
	LUA->SetField( -2, "Get" );

	LUA->PushCFunction( GetCount );
	LUA->SetField( -2, "GetCount" );

	LUA->SetField( GarrysMod::Lua::INDEX_GLOBAL, table_name );
}

void Deinitialize( lua_State *state )
{
	LUA->PushNil( );
	LUA->SetField( GarrysMod::Lua::INDEX_GLOBAL, table_name );
}

}
