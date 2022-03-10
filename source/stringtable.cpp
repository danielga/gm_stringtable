#include "stringtable.hpp"
#include "stringtablecontainer.hpp"
#include "hackednetworkstringtable.h"

#include <GarrysMod/Lua/Interface.h>
#include <lua.hpp>

#include <cstdint>
#include <algorithm>

void CNetworkStringTable::Dump( )
{
	ConMsg( "Table %s\n", GetTableName( ) );
	ConMsg( "  %i/%i items\n", GetNumStrings( ), GetMaxStrings( ) );

	for( int32_t i = 0; i < GetNumStrings( ); ++i )
		ConMsg( "  %i : %s\n", i, GetString( i ) );

	if( m_pItemsClientSide )
		for( int32_t i = 0; i < static_cast<int32_t>( m_pItemsClientSide->Count( ) ); ++i )
			ConMsg( "  %i : %s\n", i, m_pItemsClientSide->String( i ) );

	ConMsg( "\n" );
}

namespace stringtable
{

struct Container
{
	CNetworkStringTable *stringtable;
	char *name_original;
	char name[64];
};

static const char metaname[] = "stringtable";
static int32_t metatype = GarrysMod::Lua::Type::NONE;
static const char invalid_error[] = "invalid stringtable";
static const char table_name[] = "stringtables_objects";

inline void CheckType( GarrysMod::Lua::ILuaBase *LUA, int32_t index )
{
	if( !LUA->IsType( index, metatype ) )
		luaL_typerror( LUA->GetState( ), index, metaname );
}

inline Container *GetUserdata( GarrysMod::Lua::ILuaBase *LUA, int index )
{
	return LUA->GetUserType<Container>( index, metatype );
}

static CNetworkStringTable *Get( GarrysMod::Lua::ILuaBase *LUA, int32_t index )
{
	CheckType( LUA, index );
	Container *udata = GetUserdata( LUA, index );
	if( udata == nullptr )
		LUA->ArgError( index, invalid_error );

	return udata->stringtable;
}

void Push( GarrysMod::Lua::ILuaBase *LUA, CNetworkStringTable *stringtable )
{
	if( stringtable == nullptr )
	{
		LUA->PushNil( );
		return;
	}

	LUA->GetField( GarrysMod::Lua::INDEX_REGISTRY, table_name );
	LUA->PushUserdata( stringtable );
	LUA->GetTable( -2 );
	if( LUA->IsType( -1, metatype ) )
	{
		LUA->Remove( -2 );
		return;
	}

	LUA->Pop( 1 );

	Container *udata = LUA->NewUserType<Container>( metatype );
	udata->stringtable = stringtable;
	udata->name_original = stringtable->m_pszTableName;

	LUA->PushMetaTable( metatype );
	LUA->SetMetaTable( -2 );

	LUA->CreateTable( );
	lua_setfenv( LUA->GetState( ), -2 );

	LUA->PushUserdata( stringtable );
	LUA->Push( -2 );
	LUA->SetTable( -4 );
	LUA->Remove( -2 );
}

static void Destroy( GarrysMod::Lua::ILuaBase *LUA, int32_t index )
{
	Container *udata = GetUserdata( LUA, index );
	if( udata == nullptr )
		return;

	CNetworkStringTable *stringtable = udata->stringtable;

	LUA->GetField( GarrysMod::Lua::INDEX_REGISTRY, table_name );
	LUA->PushUserdata( stringtable );
	LUA->PushNil( );
	LUA->SetTable( -3 );
	LUA->Pop( 1 );

	stringtable->m_pszTableName = udata->name_original;

	LUA->SetUserType( index, nullptr );
}

LUA_FUNCTION_STATIC( gc )
{
	if( !LUA->IsType( 1, metatype ) )
		return 0;

	Destroy( LUA, 1 );
	return 0;
}

LUA_FUNCTION_STATIC( tostring )
{
	lua_pushfstring( LUA->GetState( ), "%s: %p", metaname, Get( LUA, 1 ) );
	return 1;
}

LUA_FUNCTION_STATIC( eq )
{
	LUA->PushBool( Get( LUA, 1 ) == Get( LUA, 2 ) );
	return 1;
}

LUA_FUNCTION_STATIC( index )
{
	LUA->GetMetaTable( 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	if( !LUA->IsType( -1, GarrysMod::Lua::Type::NIL ) )
		return 1;

	lua_getfenv( LUA->GetState( ), 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	return 1;
}

LUA_FUNCTION_STATIC( newindex )
{
	lua_getfenv( LUA->GetState( ), 1 );
	LUA->Push( 2 );
	LUA->Push( 3 );
	LUA->RawSet( -3 );
	return 0;
}

LUA_FUNCTION_STATIC( SetName )
{
	Container *udata = GetUserdata( LUA, 1 );
	if( udata == nullptr )
		LUA->ThrowError( invalid_error );

	V_strncpy( udata->name, LUA->CheckString( 2 ), sizeof( udata->name ) );
	udata->stringtable->m_pszTableName = udata->name;

	return 0;
}

LUA_FUNCTION_STATIC( GetName )
{
	LUA->PushString( Get( LUA, 1 )->GetTableName( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetID )
{
	LUA->PushNumber( Get( LUA, 1 )->GetTableId( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetNumStrings )
{
	LUA->PushNumber( Get( LUA, 1 )->GetNumStrings( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetMaxStrings )
{
	LUA->PushNumber( Get( LUA, 1 )->GetMaxStrings( ) );
	return 1;
}

LUA_FUNCTION_STATIC( GetEntryBits )
{
	LUA->PushNumber( Get( LUA, 1 )->GetEntryBits( ) );
	return 1;
}

LUA_FUNCTION_STATIC( SetTick )
{
	Get( LUA, 1 )->SetTick( static_cast<int32_t>( LUA->CheckNumber( 2 ) ) );
	return 0;
}

LUA_FUNCTION_STATIC( ChangedSinceTick )
{
	LUA->PushBool( Get( LUA, 1 )->ChangedSinceTick( static_cast<int32_t>( LUA->CheckNumber( 2 ) ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( AddString )
{
	INetworkStringTable *stable = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::BOOL );
	LUA->CheckType( 3, GarrysMod::Lua::Type::STRING );

	LUA->PushNumber( stable->AddString( LUA->GetBool( 2 ), LUA->GetString( 3 ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( SetString )
{
	CNetworkStringTable *stable = Get( LUA, 1 );
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
	INetworkStringTable *stable = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	const char *str = stable->GetString( static_cast<int32_t>( LUA->GetNumber( 2 ) ) );
	if( str == nullptr )
		return 0;

	LUA->PushString( str );
	return 1;
}

LUA_FUNCTION_STATIC( DeleteString )
{
	CNetworkStringTable *stable = Get( LUA, 1 );
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
		std::swap( linkedlist[k].m_value, linkedlist[k + 1].m_value );
	}

	linkedlist.Remove( max );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( SetStringUserData )
{
	INetworkStringTable *stable = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );
	LUA->CheckType( 3, GarrysMod::Lua::Type::STRING );

	unsigned int len = 0;
	const char *userdata = LUA->GetString( 3, &len );
	stable->SetStringUserData( static_cast<int32_t>( LUA->GetNumber( 2 ) ), static_cast<int32_t>( len ), userdata );
	return 0;
}

LUA_FUNCTION_STATIC( GetStringUserData )
{
	INetworkStringTable *stable = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	int32_t len = 0;
	const char *userdata = static_cast<const char *>( stable->GetStringUserData( static_cast<int32_t>( LUA->GetNumber( 2 ) ), &len ) );
	LUA->PushString( userdata, len );
	return 1;
}

LUA_FUNCTION_STATIC( GetStringUserDataInt )
{
	INetworkStringTable *stable = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	int32_t len = 0;
	const void *userdata = stable->GetStringUserData( static_cast<int32_t>( LUA->GetNumber( 2 ) ), &len );

	LUA->PushNumber( *(unsigned int *)userdata );
	return 1;
}

LUA_FUNCTION_STATIC( FindStringIndex )
{
	LUA->PushNumber( Get( LUA, 1 )->FindStringIndex( LUA->CheckString( 2 ) ) );
	return 1;
}

LUA_FUNCTION_STATIC( SetAllowClientSideAddString )
{
	INetworkStringTable *stable = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::BOOL );

	stringtablecontainer::stcinternal->SetAllowClientSideAddString( stable, LUA->GetBool( 2 ) );
	return 0;
}

LUA_FUNCTION_STATIC( DeleteAllStrings )
{
	CNetworkStringTable *stable = Get( LUA, 1 );

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

LUA_FUNCTION_STATIC( GetTable )
{
	INetworkStringTable *stable = Get( LUA, 1 );

	LUA->CreateTable( );

	for( int32_t i = 0; i < stable->GetNumStrings( ); ++i )
	{
		LUA->PushString( stable->GetString( i ) );
		int32_t len = 0;
		const char *userdata = static_cast<const char *>( stable->GetStringUserData( i, &len ) );
		LUA->PushString( userdata, len );
		LUA->SetTable( -3 );
	}

	return 1;
}

LUA_FUNCTION_STATIC( GetStrings )
{
	INetworkStringTable *stable = Get( LUA, 1 );

	LUA->CreateTable( );

	for( int32_t i = 0; i < stable->GetNumStrings( ); ++i )
	{
		LUA->PushNumber( i );
		LUA->PushString( stable->GetString( i ) );
		LUA->SetTable( -3 );
	}

	return 1;
}

LUA_FUNCTION_STATIC( GetStringsUserData )
{
	INetworkStringTable *stable = Get( LUA, 1 );

	LUA->CreateTable( );

	for( int32_t i = 0; i < stable->GetNumStrings( ); ++i )
	{
		LUA->PushNumber( i );
		int32_t len = 0;
		const char *userdata = static_cast<const char *>( stable->GetStringUserData( i, &len ) );
		LUA->PushString( userdata, len );
		LUA->SetTable( -3 );
	}

	return 1;
}

LUA_FUNCTION_STATIC( Dump )
{
	Get( LUA, 1 )->Dump( );
	return 0;
}

LUA_FUNCTION_STATIC( Lock )
{
	CNetworkStringTable *stable = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::BOOL );

	stable->Lock( LUA->GetBool( 2 ) );
	return 0;
}

void Initialize( GarrysMod::Lua::ILuaBase *LUA )
{
	LUA->CreateTable( );
	LUA->SetField( GarrysMod::Lua::INDEX_REGISTRY, table_name );

	metatype = LUA->CreateMetaTable( metaname );

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

	LUA->PushCFunction( GetStringUserDataInt );
	LUA->SetField( -2, "GetStringUserDataInt" );

	LUA->PushCFunction(FindStringIndex);
	LUA->SetField( -2, "FindStringIndex" );

	LUA->PushCFunction( SetAllowClientSideAddString );
	LUA->SetField( -2, "SetAllowClientSideAddString" );

	LUA->PushCFunction( DeleteAllStrings );
	LUA->SetField( -2, "DeleteAllStrings" );

	LUA->PushCFunction( GetTable );
	LUA->SetField( -2, "GetTable" );

	LUA->PushCFunction( GetStrings );
	LUA->SetField( -2, "GetStrings" );

	LUA->PushCFunction( GetStringsUserData );
	LUA->SetField( -2, "GetStringsUserData" );

	LUA->PushCFunction( Dump );
	LUA->SetField( -2, "Dump" );

	LUA->PushCFunction( Lock );
	LUA->SetField( -2, "Lock" );

	LUA->Pop( 1 );
}

void Deinitialize( GarrysMod::Lua::ILuaBase *LUA )
{
	LUA->PushNil( );
	LUA->SetField( GarrysMod::Lua::INDEX_REGISTRY, metaname );

	LUA->GetField( GarrysMod::Lua::INDEX_REGISTRY, table_name );
	if( LUA->IsType( -1, GarrysMod::Lua::Type::TABLE ) )
	{
		LUA->PushNil( );
		while( LUA->Next( -2 ) != 0 )
		{
			Destroy( LUA, -1 );
			LUA->Pop( 1 );
		}
	}

	LUA->Pop( 1 );

	LUA->PushNil( );
	LUA->SetField( GarrysMod::Lua::INDEX_REGISTRY, table_name );
}

}
