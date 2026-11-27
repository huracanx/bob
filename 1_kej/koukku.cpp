#include "pch.h"

//#ifdef _WIN64

LONG hook::install(void** ppPointer, void* pDetour)
{
    LONG status = DetourTransactionBegin();
    if (status != NO_ERROR) return status;
    status = DetourAttach(ppPointer, pDetour);
    if (status != NO_ERROR) { DetourTransactionAbort(); return status; }
    return DetourTransactionCommit();
}
LONG hook::remove(PVOID* ppPointer, PVOID pDetour)
{
	DetourTransactionBegin();
	DetourDetach(ppPointer, pDetour);
	return DetourTransactionCommit();
}
void hook::nop(std::uintptr_t address)
{
	write_addr(address, "\x90\x90\x90\x90\x90", 5);
}
void hook::write_addr(void* addr, const char* bytes, size_t len)
{
	write_addr(addr, (void*)bytes, len);
}
void hook::write_addr(void* addr, void* bytes, size_t len)
{
	DWORD oldProtect;

	VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(addr, bytes, len);
#pragma warning(suppress: 6387)
	VirtualProtect((LPVOID)addr, len, oldProtect, NULL);
}
void hook::write_addr(std::uintptr_t addr, const char* bytes, size_t len)
{
	return write_addr((void*)addr, (void*)bytes, len);
}
void hook::memcopy(void* dst, void* src, size_t len)
{
	return write_addr(dst, src, len);
}
void hook::get_bytes(void* addr, size_t len, BYTE* buffer)
{
	std::stringstream ss;
	std::string string;

	for (uint32_t i = 0; i < len; i++)
	{
		string = std::format("{:#x}", *(BYTE*)((size_t)addr + i));
		string.erase(0, 2);
		std::istringstream str(string);

		uint16_t val{};
		str >> std::hex >> val;
		buffer[i] = (BYTE)val;
	}

}
std::uintptr_t hook::find_pattern(std::string moduleName, std::string pattern)
{
    const HMODULE module = GetModuleHandleA(moduleName.c_str());
    MODULEINFO info{};
    if (!module || !K32GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info)))
        return 0;
    const DWORD begin = reinterpret_cast<DWORD>(module);
    return find_pattern(begin, begin + info.SizeOfImage, pattern);
}

std::uintptr_t hook::find_pattern(DWORD start_address, DWORD end_address, std::string pattern)
{
    std::istringstream stream(pattern);
    std::vector<int> bytes;
    for (std::string token; stream >> token; ) {
        if (token == "?" || token == "??") bytes.push_back(-1);
        else if (token.size() == 2) bytes.push_back(getByte(token.c_str()));
        else return 0;
    }
    if (bytes.empty() || end_address <= start_address) return 0;
    for (DWORD region = start_address; region < end_address; ) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<void*>(region), &mbi, sizeof(mbi))) break;
        const DWORD next = static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize);
        if (next <= region) break;
        const DWORD limit = next < end_address ? next : end_address;
        const DWORD protection = mbi.Protect & 0xff;
        const bool readable = protection == PAGE_READONLY || protection == PAGE_READWRITE ||
            protection == PAGE_WRITECOPY || protection == PAGE_EXECUTE_READ ||
            protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
        if (mbi.State == MEM_COMMIT && readable && !(mbi.Protect & PAGE_GUARD) &&
            static_cast<size_t>(limit - region) >= bytes.size()) {
            const DWORD last = limit - static_cast<DWORD>(bytes.size());
            for (DWORD pos = region; pos <= last; ++pos) {
                const BYTE* data = reinterpret_cast<const BYTE*>(pos);
                size_t i = 0;
                while (i < bytes.size() && (bytes[i] < 0 || data[i] == bytes[i])) ++i;
                if (i == bytes.size()) return pos;
            }
        }
        region = next;
    }
    return 0;
}
