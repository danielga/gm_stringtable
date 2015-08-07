#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/AutoReference.h>
#include <lua.hpp>
#include <interfaces.hpp>
#include <hackednetworkstringtable.h>
#include <cstdint>

namespace global
{

static SourceSDK::FactoryLoader engine_loader( "engine", false, IS_SERVERSIDE );

#if defined STRINGTABLE_SERVER

static const char *networkstringtable_interface = INTERFACENAME_NETWORKSTRINGTABLESERVER;

#elif defined STRINGTABLE_CLIENT

static const char *networkstringtable_interface = INTERFACENAME_NETWORKSTRINGTABLECLIENT;

#endif

static CNetworkStringTableContainer *stringtablecontainer = nullptr;

static void Initialize( lua_State *state )
{
	stringtablecontainer = engine_loader.GetInterface<CNetworkStringTableContainer>( networkstringtable_interface );
	if( stringtablecontainer == nullptr )
		LUA->ThrowError( "INetworkStringTableContainer not initialized. Critical error." );
}

}

namespace stringtable
{

struct UserData
{
	CNetworkStringTable *stringtable;
	uint8_t type;
};

static const char *metaname = "stringtable";
static const uint8_t metatype = 200;
static const char *invalid_error = "invalid stringtable";

static GarrysMod::Lua::AutoReference stringtables_names;

inline void CheckType( lua_State *state, int32_t index )
{
	if( !LUA->IsType( index, metatype ) )
		luaL_typerror( state, index, metaname );
}

inline void Create( lua_State *state, CNetworkStringTable *stable )
{
	UserData *udata = static_cast<UserData *>( LUA->NewUserdata( sizeof( UserData ) ) );
	udata->stringtable = stable;
	udata->type = metatype;

	LUA->CreateMetaTableType( metaname, metatype );
	LUA->SetMetaTable( -2 );
}

inline UserData *GetUserdata( lua_State *state, int index )
{
	return static_cast<UserData *>( LUA->GetUserdata( index ) );
}

static CNetworkStringTable *Get( lua_State *state, int32_t index )
{
	CheckType( state, index );
	CNetworkStringTable *stable = static_cast<UserData *>( LUA->GetUserdata( index ) )->stringtable;
	if( stable == nullptr )
		LUA->ArgError( index, invalid_error );

	return stable;
}

LUA_FUNCTION_STATIC( gc )
{
	LUA->CheckType( 1, metatype );
	GetUserdata( state, 1 )->stringtable = nullptr;
	return 0;
}

LUA_FUNCTION_STATIC( tostring )
{
	lua_pushfstring( state, "%s: %p", metaname, Get( state, 1 ) );
	return 1;
}

LUA_FUNCTION_STATIC( eq )
{
	LUA->PushBool( Get( state, 1 ) == Get( state, 2 ) );
	return 1;
}

LUA_FUNCTION_STATIC( index )
{
	LUA->GetMetaTable( 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	if( !LUA->IsType( -1, GarrysMod::Lua::Type::NIL ) )
		return 1;

	LUA->Pop( 2 );

	lua_getfenv( state, 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	return 1;
}

LUA_FUNCTION_STATIC( newindex )
{
	lua_getfenv( state, 1 );
	LUA->Push( 2 );
	LUA->Push( 3 );
	LUA->RawSet( -3 );
	return 0;
}

LUA_FUNCTION_STATIC( SetName )
{
	CNetworkStringTable *stable = Get( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::STRING );

	const char *name = LUA->GetString( 2 );
	if( global::stringtablecontainer->FindTable( name ) != nullptr )
	{
		LUA->PushBool( false );
		return 1;
	}

	char *original = stable->m_pszTableName;

	stringtables_names.Push( );

	LUA->GetField( -1, original );
	if( LUA->IsType( -1, GarrysMod::Lua::Type::LIGHTUSERDATA ) )
	{
		char *_original = static_cast<char *>( LUA->GetUserdata( -1 ) );

		LUA->PushNil( );
		LUA->SetField( -2, original );

		original = _original;
	}

	LUA->Pop( 1 );

	LUA->PushString( name );
	LUA->Push( -1 );
	LUA->PushUserdata( original );
	LUA->SetTable( -4 );

	stable->m_pszTableName = const_cast<char *>( LUA->GetString( -1 ) );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( GetName )
{
	LUA->PushString( Get( state, 1 )->GetTableName( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetID )
{
	LUA->PushNumber( Get( state, 1 )->GetTableId( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetNumStrings )
{
	LUA->PushNumber( Get( state, 1 )->GetNumStrings( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetMaxStrings )
{
	LUA->PushNumber( Get( state, 1 )->GetMaxStrings( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetEntryBits )
{
	LUA->PushNumber( Get( state, 1 )->GetEntryBits( ) );
	return 1;
}

LUA_FUNCTION_STATIC( SetTick )
{
	Get( state, 1 )->SetTick( static_cast<int32_t>( LUA->CheckNumber( 2 ) ) );
	return 0;
}

LUA_FUNCTION_STATIC( ChangedSinceTick )
{
	LUA->PushBool( Get( state, 1 )->ChangedSinceTick( static_cast<int32_t>( LUA->CheckNumber( 2 ) ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( AddString )
{
	INetworkStringTable *stable = Get( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::BOOL );
	LUA->CheckType( 3, GarrysMod::Lua::Type::STRING );

	LUA->PushNumber( stable->AddString( LUA->GetBool( 2 ), LUA->GetString( 3 ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( SetString )
{
	CNetworkStringTable *stable = Get( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );
	LUA->CheckType( 3, GarrysMod::Lua::Type::STRING );

	uint32_t index = static_cast<uint32_t>( LUA->GetNumber( 2 ) );

	const char *str = LUA->GetString( 3 );
	if( stable->FindStringIndex( str ) != INVALID_STRING_INDEX )
	{
		LUA->PushBool( false );
		return 1;
	}

	CNetworkStringDict *networkdict = stable->m_pItems;
	if( networkdict == nullptr )
	{
		LUA->PushBool( false );
		return 1;
	}

	CNetworkStringDict::StableHashtable_t &dict = networkdict->m_Items;
	if( !dict.IsValidHandle( index ) )
	{
		LUA->PushBool( false );
		return 1;
	}

	dict.ReplaceKey( index, str );
	dict.Element( index ).m_nTickCreated = stable->m_nTickCount + 5;

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( GetString )
{
	INetworkStringTable *stable = Get( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	const char *str = stable->GetString( static_cast<int32_t>( LUA->GetNumber( 2 ) ) );
	if( str == nullptr )
		return 0;

	LUA->PushString( str );
	return 1;
}

LUA_FUNCTION_STATIC( DeleteString )
{
	CNetworkStringTable *stable = Get( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	uint32_t index = static_cast<uint32_t>( LUA->GetNumber( 2 ) );

	CNetworkStringDict *networkdict = stable->m_pItems;
	if( networkdict == nullptr )
	{
		LUA->PushBool( false );
		return 1;
	}

	CNetworkStringDict::_StableHashtable_t &dict = static_cast<CNetworkStringDict::_StableHashtable_t &>( networkdict->m_Items );
	if( !dict.IsValidHandle( index ) )
	{
		LUA->PushBool( false );
		return 1;
	}

	uint32_t max = dict.Count( ) - 1;

	auto &hashtable = dict.GetHashTable( );
	UtlHashHandle_t first = hashtable.FirstHandle( ), idx = hashtable.InvalidHandle( );
	for( UtlHashHandle_t k = first; k != hashtable.InvalidHandle( ); k = hashtable.NextHandle( k ) )
		if( hashtable.Key( k ).m_index == max )
			idx = k;

	hashtable.RemoveAndAdvance( idx != hashtable.InvalidHandle( ) ? idx : first );

	auto &linkedlist = dict.GetLinkedList( );
	for( uint32_t k = index; k < max; ++k )
	{
		linkedlist[k].m_key = linkedlist[k + 1].m_key;
		V_swap( linkedlist[k].m_value, linkedlist[k + 1].m_value );
	}

	linkedlist.Remove( max );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( SetStringUserData )
{
	INetworkStringTable *stable = Get( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );
	LUA->CheckType( 3, GarrysMod::Lua::Type::STRING );

	size_t len = 0;
	const char *userdata = LUA->GetString( 3, &len );
	stable->SetStringUserData( static_cast<int32_t>( LUA->GetNumber( 2 ) ), static_cast<int32_t>( len ), userdata );
	return 0;
}

LUA_FUNCTION_STATIC( GetStringUserData )
{
	INetworkStringTable *stable = Get( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	int32_t len = 0;
	const char *userdata = static_cast<const char *>( stable->GetStringUserData( static_cast<int32_t>( LUA->GetNumber( 2 ) ), &len ) );
	LUA->PushString( userdata, len );
	return 1;
}

LUA_FUNCTION_STATIC( FindStringIndex )
{
	LUA->PushNumber( Get( state, 1 )->FindStringIndex( LUA->CheckString( 2 ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( SetAllowClientSideAddString )
{
	INetworkStringTable *stable = Get( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::BOOL );

	global::stringtablecontainer->SetAllowClientSideAddString( stable, LUA->GetBool( 2 ) );
	return 0;
}

LUA_FUNCTION_STATIC( DeleteAllStrings )
{
	CNetworkStringTable *stable = Get( state, 1 );

	CNetworkStringDict *networkdict = stable->m_pItems;
	if( networkdict == nullptr || networkdict->Count( ) == 0 )
	{
		LUA->PushBool( false );
		return 1;
	}

	networkdict->Purge( );

	LUA->PushBool( true );
	return 1;
}

static void Initialize( lua_State *state )
{
	stringtables_names.Setup( LUA );
	LUA->CreateTable( );
	stringtables_names.Create( );

	LUA->CreateMetaTableType( metaname, metatype );

	LUA->PushCFunction( gc );
	LUA->SetField( -2, "__gc" );

	LUA->PushCFunction( tostring );
	LUA->SetField( -2, "__tostring" );

	LUA->PushCFunction( eq );
	LUA->SetField( -2, "__eq" );

	LUA->PushCFunction( index );
	LUA->SetField( -2, "__index" );

	LUA->PushCFunction( newindex );
	LUA->SetField( -2, "__newindex" );

	LUA->PushCFunction( SetName );
	LUA->SetField( -2, "SetName" );

	LUA->PushCFunction( GetName );
	LUA->SetField( -2, "GetName" );

	LUA->PushCFunction( GetID );
	LUA->SetField( -2, "GetID" );

	LUA->PushCFunction( GetNumStrings );
	LUA->SetField( -2, "GetNumStrings" );

	LUA->PushCFunction( GetMaxStrings );
	LUA->SetField( -2, "GetMaxStrings" );

	LUA->PushCFunction( GetEntryBits );
	LUA->SetField( -2, "GetEntryBits" );

	LUA->PushCFunction( SetTick );
	LUA->SetField( -2, "SetTick" );

	LUA->PushCFunction( ChangedSinceTick );
	LUA->SetField( -2, "ChangedSinceTick" );

	LUA->PushCFunction( AddString );
	LUA->SetField( -2, "AddString" );

	LUA->PushCFunction( SetString );
	LUA->SetField( -2, "SetString" );

	LUA->PushCFunction( GetString );
	LUA->SetField( -2, "GetString" );

	LUA->PushCFunction( DeleteString );
	LUA->SetField( -2, "DeleteString" );

	LUA->PushCFunction( SetStringUserData );
	LUA->SetField( -2, "SetStringUserData" );

	LUA->PushCFunction( GetStringUserData );
	LUA->SetField( -2, "GetStringUserData" );

	LUA->PushCFunction( FindStringIndex );
	LUA->SetField( -2, "FindStringIndex" );

	LUA->PushCFunction( SetAllowClientSideAddString );
	LUA->SetField( -2, "SetAllowClientSideAddString" );

	LUA->PushCFunction( DeleteAllStrings );
	LUA->SetField( -2, "DeleteAllStrings" );

	LUA->Pop( 1 );
}

static void Deinitialize( lua_State *state )
{
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_REG );

	LUA->PushNil( );
	LUA->SetField( -2, metaname );

	LUA->Pop( 1 );

	stringtables_names.Push( );

	CUtlVector<CNetworkStringTable *> &tables = global::stringtablecontainer->m_Tables;
	for( int32_t k = 0; k < tables.Count( ); ++k )
	{
		CNetworkStringTable *table = tables[k];
		LUA->GetField( -1, table->m_pszTableName );
		if( LUA->IsType( -1, GarrysMod::Lua::Type::LIGHTUSERDATA ) )
			table->m_pszTableName = static_cast<char *>( LUA->GetUserdata( -1 ) );
	}

	LUA->Pop( 1 );
	stringtables_names.Free( );
}

}

namespace stringtables
{

LUA_FUNCTION_STATIC( Find )
{
	LUA->CheckType( 1, GarrysMod::Lua::Type::STRING );

	CNetworkStringTable *stable = static_cast<CNetworkStringTable *>(
		global::stringtablecontainer->FindTable( LUA->GetString( 1 ) )
	);
	if( stable == nullptr )
		return 0;

	stringtable::Create( state, stable );
	return 1;
}

LUA_FUNCTION_STATIC( Get )
{
	LUA->CheckType( 1, GarrysMod::Lua::Type::NUMBER );

	CNetworkStringTable *stable = static_cast<CNetworkStringTable *>(
		global::stringtablecontainer->GetTable( static_cast<int>( LUA->GetNumber( 1 ) ) )
	);
	if( stable == nullptr )
		return 0;

	stringtable::Create( state, stable );
	return 1;
}

LUA_FUNCTION_STATIC( GetCount )
{
	LUA->PushNumber( global::stringtablecontainer->GetNumTables( ) );
	return 1;
}

static void Initialize( lua_State *state )
{
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->CreateTable( );

	LUA->PushCFunction( Find );
	LUA->SetField( -2, "Find" );

	LUA->PushCFunction( Get );
	LUA->SetField( -2, "Get" );

	LUA->PushCFunction( GetCount );
	LUA->SetField( -2, "GetCount" );

	LUA->SetField( -2, "stringtable" );

	LUA->Pop( 1 );
}

static void Deinitialize( lua_State *state )
{
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->PushNil( );
	LUA->SetField( -2, "stringtable" );

	LUA->Pop( 1 );
}

}

GMOD_MODULE_OPEN( )
{
	global::Initialize( state );
	stringtables::Initialize( state );
	stringtable::Initialize( state );
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	stringtable::Deinitialize( state );
	stringtables::Deinitialize( state );
	return 0;
}
