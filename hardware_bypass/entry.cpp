#include <cstdint>
#include <cstdio>
#include <Windows.h>

#define LOGGER_NAME "[ HARDWARE BYPASS ] "
#define TRACE( str, ... ) printf_s( LOGGER_NAME str"\n", __VA_ARGS__ )

bool patch_runtime( )
{
	auto patch = [ & ]( uintptr_t const address ) -> bool
	{
		DWORD old_protect = 0;
		bool result = VirtualProtect( reinterpret_cast< void* >( address ), sizeof( void* ), PAGE_EXECUTE_READWRITE, &old_protect );
		if ( !result )
			return false;

		*reinterpret_cast< uint8_t* >( address ) = 0xC3;
		result = VirtualProtect( reinterpret_cast< void* >( address ), sizeof( void* ), old_protect, &old_protect );

		return result;
	};

	auto const game_module_address = GetModuleHandleA( "UAGame.exe" );
	if ( !game_module_address )
	{
		TRACE( "Failed to find \"UAGame.exe\" module" );
		return false;
	}
	TRACE( "UAGame.exe address = 0x%p", game_module_address );

	// @ida: signature: 48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8B EC 48 83 EC ? 8B 79
	/* @ida: string: FWindowsDevicePermissionModule check hardware macro on
	*  mov     rcx, rbx
    *  add     rsp, 20h
    *  pop     rbx
	*  jmp     sub_1419AC180 <--- our function to patch
	*/
	auto const function_address = reinterpret_cast< uintptr_t >( game_module_address ) + 0x19AC180;

	TRACE( "Function address: 0x%llx", function_address );

	if ( patch( function_address  ) )
	{
		TRACE( "Hardware checks patched success" );
	}
	else
	{
		TRACE( "Failed to patch functions. Last error: 0x%x", GetLastError( ) );
		return false;
	}

	return true;
}

// ReSharper disable once CppInconsistentNaming
bool WINAPI DllMain( [[maybe_unused]] void* instance, uint32_t	const call_reason, [[maybe_unused]] void* reserved )
{
	if ( call_reason != DLL_PROCESS_ATTACH )
		return false;

	if ( AllocConsole( ) )
	{
		SetConsoleTitleA( "HARDWARE PATCHER" );

		FILE* file{};
		freopen_s( &file, "CONOUT$", "w+", stdout );
	}

	TRACE( "Initialization..." );

	patch_runtime( );

	return true;
}

