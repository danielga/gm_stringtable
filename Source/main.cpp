#include <GarrysMod/Lua/Interface.h>
#include <interface.h>
#include <hackednetworkstringtable.h>
#include <cstdint>

namespace global
{

static CDllDemandLoader engine_loader( "engine.dll" );

#if defined STRINGTABLES_SERVER

static const char *networkstringtable_interface = INTERFACENAME_NETWORKSTRINGTABLESERVER;

#elif defined STRINGTABLES_CLIENT

static const char *networkstringtable_interface = INTERFACENAME_NETWORKSTRINGTABLECLIENT;

#endif

static INetworkStringTableContainer *stringtablecontainer = nullptr;

static void Initialize( lua_State *state )
{
	CreateInterfaceFn factory = engine_loader.GetFactory( );
	if( factory == nullptr )
		LUA->ThrowError( "Couldn't get engine factory. Critical error." );

	stringtablecontainer = static_cast<INetworkStringTableContainer *>( factory( networkstringtable_interface, NULL ) );
	if( stringtablecontainer == nullptr )
		LUA->ThrowError( "INetworkStringTableContainer not initialized. Critical error." );
}

}

namespace stringtable
{

struct userdata
{
	CNetworkStringTable *data;
	uint8_t type;
};

typedef CUtlStableHashtable<
	CUtlConstString,
	CNetworkStringTableItem,
	CaselessStringHashFunctor,
	UTLConstStringCaselessStringEqualFunctor<char>,
	uint16,
	const char *
> CNetworkStringDictStableHashtable;

static const char *metaname = "stringtable";
static const uint8_t metatype = 200;

static const char *invalid_error = "stringtable is not valid";

static userdata *Create( lua_State *state )
{
	userdata *udata = static_cast<userdata *>( LUA->NewUserdata( sizeof( userdata ) ) );
	udata->type = metatype;

	LUA->CreateMetaTableType( metaname, metatype );
	LUA->SetMetaTable( -2 );

	return udata;
}

inline userdata *GetUserdata( lua_State *state, int index )
{
	return static_cast<userdata *>( LUA->GetUserdata( index ) );
}

inline CNetworkStringTable *GetAndValidate( lua_State *state, int index, const char *err )
{
	CNetworkStringTable *stable = GetUserdata( state, index )->data;
	if( stable == nullptr )
		LUA->ThrowError( err );

	return stable;
}

LUA_FUNCTION_STATIC( gc )
{
	LUA->CheckType( 1, metatype );

	userdata *udata = GetUserdata( state, 1 );
	udata->data = nullptr;
	return 0;
}

LUA_FUNCTION_STATIC( tostring )
{
	LUA->CheckType( 1, metatype );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	char formatted[30] = { 0 };
	V_snprintf( formatted, sizeof( formatted ), "%s: 0x%p", metaname, stable );
	LUA->PushString( formatted );

	return 1;
}

LUA_FUNCTION_STATIC( eq )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, metatype );

	INetworkStringTable *stable1 = GetAndValidate( state, 1, invalid_error );
	INetworkStringTable *stable2 = GetAndValidate( state, 2, invalid_error );

	LUA->PushBool( stable1 == stable2 );
	return 1;
}

LUA_FUNCTION_STATIC( SetName )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::STRING );

	CNetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	const char *name = LUA->GetString( 2 );
	if( global::stringtablecontainer->FindTable( name ) != nullptr )
	{
		LUA->PushBool( false );
		return 1;
	}

	if( stable->m_pszTableName != nullptr )
		delete [] stable->m_pszTableName;

	size_t len = V_strlen( name ) + 1;
	stable->m_pszTableName = new char[len];
	V_strncpy( stable->m_pszTableName, name, len );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( GetName )
{
	LUA->CheckType( 1, metatype );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	LUA->PushString( stable->GetTableName( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetID )
{
	LUA->CheckType( 1, metatype );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	LUA->PushNumber( stable->GetTableId( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetNumStrings )
{
	LUA->CheckType( 1, metatype );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	LUA->PushNumber( stable->GetNumStrings( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetMaxStrings )
{
	LUA->CheckType( 1, metatype );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	LUA->PushNumber( stable->GetMaxStrings( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetEntryBits )
{
	LUA->CheckType( 1, metatype );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	LUA->PushNumber( stable->GetEntryBits( ) );
	return 1;
}

LUA_FUNCTION_STATIC( SetTick )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	stable->SetTick( static_cast<int>( LUA->GetNumber( 2 ) ) );
	return 0;
}

LUA_FUNCTION_STATIC( ChangedSinceTick )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	LUA->PushBool( stable->ChangedSinceTick( static_cast<int>( LUA->GetNumber( 2 ) ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( AddString )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::BOOL );
	LUA->CheckType( 3, GarrysMod::Lua::Type::STRING );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	LUA->PushNumber( stable->AddString( LUA->GetBool( 2 ), LUA->GetString( 3 ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( SetString )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );
	LUA->CheckType( 3, GarrysMod::Lua::Type::STRING );

	CNetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	int index = static_cast<int>( LUA->GetNumber( 2 ) );

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

	CNetworkStringDictStableHashtable &dict = networkdict->m_Items;
	if( !dict.IsValidHandle( index ) )
	{
		LUA->PushBool( false );
		return 1;
	}

	dict.ReplaceKey( index, CUtlConstString( str ) );
	dict.Element( index ).m_nTickCreated = stable->m_nTickCount + 5;

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( GetString )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	const char *str = stable->GetString( static_cast<int>( LUA->GetNumber( 2 ) ) );
	if( str == nullptr )
		return 0;

	LUA->PushString( str );
	return 1;
}

LUA_FUNCTION_STATIC( DeleteString )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	CNetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	int index = static_cast<int>( LUA->GetNumber( 2 ) );

	CNetworkStringDict *networkdict = stable->m_pItems;
	if( networkdict == nullptr )
	{
		LUA->PushBool( false );
		return 1;
	}

	CNetworkStringDictStableHashtable &dict = networkdict->m_Items;
	if( !dict.IsValidHandle( index ) )
	{
		LUA->PushBool( false );
		return 1;
	}

	dict.RemoveAndAdvance( index );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( SetStringUserData )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );
	LUA->CheckType( 3, GarrysMod::Lua::Type::STRING );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	size_t len = 0;
	const char *userdata = LUA->GetString( 3, &len );
	stable->SetStringUserData( static_cast<int>( LUA->GetNumber( 2 ) ), static_cast<int>( len ), userdata );
	return 0;
}

LUA_FUNCTION_STATIC( GetStringUserData )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	int len = 0;
	const char *userdata = static_cast<const char *>( stable->GetStringUserData( static_cast<int>( LUA->GetNumber( 2 ) ), &len ) );
	LUA->PushString( userdata, len );
	return 1;
}

LUA_FUNCTION_STATIC( FindStringIndex )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::STRING );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	LUA->PushNumber( stable->FindStringIndex( LUA->GetString( 2 ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( SetAllowClientSideAddString )
{
	LUA->CheckType( 1, metatype );
	LUA->CheckType( 2, GarrysMod::Lua::Type::BOOL );

	INetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

	global::stringtablecontainer->SetAllowClientSideAddString( stable, LUA->GetBool( 2 ) );
	return 0;
}

LUA_FUNCTION_STATIC( DeleteAllStrings )
{
	LUA->CheckType( 1, metatype );

	CNetworkStringTable *stable = GetAndValidate( state, 1, invalid_error );

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

static void RegisterMetaTable( lua_State *state )
{
	LUA->CreateMetaTableType( metaname, metatype );

	LUA->Push( -1 );
	LUA->SetField( -2, "__index" );

	LUA->PushCFunction( gc );
	LUA->SetField( -2, "__gc" );

	LUA->PushCFunction( tostring );
	LUA->SetField( -2, "__tostring" );

	LUA->PushCFunction( eq );
	LUA->SetField( -2, "__eq" );

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

	stringtable::Create( state )->data = stable;
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

	stringtable::Create( state )->data = stable;
	return 1;
}

LUA_FUNCTION_STATIC( GetCount )
{
	LUA->PushNumber( global::stringtablecontainer->GetNumTables( ) );
	return 1;
}

static void RegisterGlobalTable( lua_State *state )
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
}

}

GMOD_MODULE_OPEN( )
{
	global::Initialize( state );

	stringtables::RegisterGlobalTable( state );

	stringtable::RegisterMetaTable( state );

	return 0;
}

GMOD_MODULE_CLOSE( )
{
	(void)state;
	return 0;
}