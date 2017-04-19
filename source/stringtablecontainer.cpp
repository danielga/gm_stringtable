#include <stringtablecontainer.hpp>
#include <stringtable.hpp>
#include <GarrysMod/Lua/Interface.h>
#include <hackednetworkstringtable.h>

#if defined max || defined min

#undef max
#undef min

#endif

#include <GarrysMod/Interfaces.hpp>
#include <cstdint>

void CNetworkStringTableContainer::Dump( )
{
	for( int32_t i = 0; i < m_Tables.Count( ); ++i )
		m_Tables[i]->Dump( );
}

bool CNetworkStringTableContainer::Lock( bool bLock )
{
	bool oldLock = m_bLocked;
	m_bLocked = bLock;

	for( int32_t i = 0; i < m_Tables.Count( ); ++i )
		reinterpret_cast<CNetworkStringTable *>( GetTable( i ) )->Lock( bLock );

	return oldLock;
}

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
	stringtable::Push( LUA, static_cast<CNetworkStringTable *>(
		stcinternal->FindTable( LUA->CheckString( 1 ) )
	) );
	return 1;
}

LUA_FUNCTION_STATIC( Get )
{
	stringtable::Push( LUA, static_cast<CNetworkStringTable *>(
		stcinternal->GetTable( static_cast<int32_t>( LUA->CheckNumber( 1 ) ) )
	) );
	return 1;
}

LUA_FUNCTION_STATIC( GetCount )
{
	LUA->PushNumber( stcinternal->GetNumTables( ) );
	return 1;
}

LUA_FUNCTION_STATIC( Lock )
{
	LUA->PushBool( stcinternal->Lock( LUA->GetBool( 1 ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( Dump )
{
	stcinternal->Dump( );
	return 0;
}

LUA_FUNCTION_STATIC( GetNames )
{
	LUA->CreateTable( );

	for( int32_t i = 0; i < stcinternal->GetNumTables( ); ++i )
	{
		LUA->PushNumber( i );
		LUA->PushString( stcinternal->GetTable( i )->GetTableName( ) );
		LUA->SetTable( -3 );
	}

	return 1;
}

void Initialize( GarrysMod::Lua::ILuaBase *LUA )
{
	stcinternal = engine_loader.GetInterface<CNetworkStringTableContainer>( networkstringtable_interface );
	if( stcinternal == nullptr )
		LUA->ThrowError( "unable to initialize INetworkStringTableContainer" );

	LUA->CreateTable( );

	LUA->PushString( "stringtable 1.1.1" );
	LUA->SetField( -2, "Version" );

	// version num follows LuaJIT style, xxyyzz
	LUA->PushNumber( 10101 );
	LUA->SetField( -2, "VersionNum" );

	LUA->PushCFunction( Find );
	LUA->SetField( -2, "Find" );

	LUA->PushCFunction( Get );
	LUA->SetField( -2, "Get" );

	LUA->PushCFunction( GetCount );
	LUA->SetField( -2, "GetCount" );

	LUA->PushCFunction( Lock );
	LUA->SetField( -2, "Lock" );

	LUA->PushCFunction( Dump );
	LUA->SetField( -2, "Dump" );

	LUA->PushCFunction( GetNames );
	LUA->SetField( -2, "GetNames" );

	LUA->SetField( GarrysMod::Lua::INDEX_GLOBAL, table_name );
}

void Deinitialize( GarrysMod::Lua::ILuaBase *LUA )
{
	LUA->PushNil( );
	LUA->SetField( GarrysMod::Lua::INDEX_GLOBAL, table_name );
}

}
