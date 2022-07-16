//
// sv_showhitboxes reimplementation client plugin
//

#include "engine/iserverplugin.h"
#include "tier1/tier1.h"
#include "eiface.h"

#ifdef _LINUX
    #include <fcntl.h>
    #include <link.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
#endif
#ifdef _WIN32
    #include <windows.h>
#endif

//
// Memory hacking utilities
//

struct ModuleInfo
{
    void*   handle;
    DWORD   baseAddress;
};

static bool BGetServerModule(ModuleInfo* pModule)
{
#ifdef _LINUX
    if ((pModule->handle = dlopen("./tf/bin/server.so", RTLD_NOW | RTLD_NOLOAD)) == NULL)
        return false;

    // Even though dlopen is called with RTLD_NOLOAD, the above call to dlopen still
    // increments glibc's internal reference counter
    dlclose(pModule->handle);

    // Module start address
    link_map* pServerLinkMap = NULL;
    if (dlinfo(pModule->handle, RTLD_DI_LINKMAP, &pServerLinkMap) == -1)
        return false;

    pModule->baseAddress = pServerLinkMap->l_addr;
#endif
#ifdef _WIN32
    if ((pModule->handle = GetModuleHandle("server.dll")) == NULL)
        return false;

    pModule->baseAddress = (DWORD)pModule->handle;
#endif
    return true;
}

static DWORD FindBytePattern(const ModuleInfo* pModule, const char* szPattern)
{
    // Linux: ld doesn't map the .text section in with readable permissions, so we have
    // to load the file from disk and scan the copy
#ifdef _LINUX
    // Get path
    char szModulePath[PATH_MAX + 1];
    if (dlinfo(pModule->handle, RTLD_DI_ORIGIN, &szModulePath) == -1)
        return 0;
    V_strncat(szModulePath, "/server.so", PATH_MAX);

    // Map file into memory
    int moduleFileDescriptor = open(szModulePath, O_RDONLY);
    if (moduleFileDescriptor == -1)
        return 0;

    struct stat moduleFileInfo;
    if (fstat(moduleFileDescriptor, &moduleFileInfo) == -1)
    {
        close(moduleFileDescriptor);
        return 0;
    }

    DWORD moduleSize = moduleFileInfo.st_size;
    BYTE* pModuleBase = (BYTE*)mmap(NULL, moduleSize, PROT_READ, MAP_PRIVATE, moduleFileDescriptor, 0);
    close(moduleFileDescriptor);
    if (pModuleBase == MAP_FAILED)
        return 0;
#endif
    // Windows: The PE header tells us where the code offset is, so just scan
    // that section
#ifdef _WIN32
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)pModule->baseAddress;
    PIMAGE_NT_HEADERS pNtHdrs = (PIMAGE_NT_HEADERS)(pModule->baseAddress + pDosHdr->e_lfanew);
    DWORD moduleSize = pNtHdrs->OptionalHeader.SizeOfCode;
    BYTE* pModuleBase = (BYTE*)(pModule->baseAddress + pNtHdrs->OptionalHeader.BaseOfCode);
#endif

    // Scan file
    DWORD result = 0;
    for (DWORD i = 0; i < moduleSize; ++i)
    {
        // Match pattern
        for (DWORD j = 0; ; ++j)
        {
            // Don't overrun
            if (i + j >= moduleSize)
                break;
            // End of pattern, successful match
            if (szPattern[j] == '\0')
            {
#ifdef _WIN32
                result = (DWORD)pModuleBase + i;
#else
                result = pModule->baseAddress + i;
#endif
                goto done;
            }
            // Compare bytes
            if (szPattern[j] != '\x2A' && (BYTE)szPattern[j] != pModuleBase[i + j])
                break;
        }
    }

    done:
    // Clean up
#ifdef _LINUX
    munmap(pModuleBase, moduleSize);
#endif
    return result;
}

//
// Plugin interface
// See: public/engine/iserverplugin.h
//
class CShowHitboxesPlugin : public IServerPluginCallbacks
{
public:
    // Called on plugin load
    virtual bool            Load(CreateInterfaceFn, CreateInterfaceFn);
    // Called on plugin unload
    virtual void            Unload();
    // Called when the plugin is suspended
    virtual void            Pause() { }
    // Called when the plugin is resumed
    virtual void            UnPause() { }
    // Called in plugin_print
    virtual const char*     GetPluginDescription();
    // Called on level load
    virtual void            LevelInit(const char*) { }
    // Called on server load
    virtual void            ServerActivate(edict_t*, int, int) { }
    // Called on game frame
    virtual void            GameFrame(bool);
    // Called on level unload
    virtual void            LevelShutdown() { }
    // Called on client spawn
    virtual void            ClientActive(edict_t*) { }
    // Called on client disconnect
    virtual void            ClientDisconnect(edict_t*) { }
    // Called on client connect
    virtual void            ClientPutInServer(edict_t*, const char*) { }
    // Called when a client runs a command
    virtual void            SetCommandClient(int) { }
    // Called when a client changes replicated cvar
    virtual void            ClientSettingsChanged(edict_t*) { }
    // Called when a client tries to connect
    virtual PLUGIN_RESULT   ClientConnect(bool*, edict_t*, const char*, const char*, char*, int) { return PLUGIN_CONTINUE; }
    // Called when a client runs a command
    virtual PLUGIN_RESULT   ClientCommand(edict_t*, const CCommand&) { return PLUGIN_CONTINUE; }
    // Called when a client receives a network ID
    virtual PLUGIN_RESULT   NetworkIDValidated(const char*, const char*) { return PLUGIN_CONTINUE; }
    // Called on client cvar query finished
    virtual void            OnQueryCvarValueFinished(QueryCvarCookie_t, edict_t*, EQueryCvarValueStatus, const char*, const char*) { }
    // Called on edict allocated
    virtual void            OnEdictAllocated(edict_t*) { }
    // Called on edict freed
    virtual void            OnEdictFreed(const edict_t*) { }
};

// Expose plugin as interface
CShowHitboxesPlugin g_Plugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CShowHitboxesPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_Plugin);

static ConVar sv_showhitboxes("sv_showhitboxes", "-1", FCVAR_CHEAT, "Send server-side hitboxes for specified entity to client (NOTE:  this uses lots of bandwidth, use on listen server only).");

#ifdef _WIN32
    #define THISCALL_CONV __thiscall
#else
    #define THISCALL_CONV
#endif

//
// Private game interfaces
//
namespace TF2
{
    class CBaseEntity : public IServerEntity
    {
    public:
        static CBaseEntity* Instance(edict_t* pEdict)
        {
            if (pEdict && pEdict->GetUnknown())
            {
                return (TF2::CBaseEntity*)pEdict->GetUnknown()->GetBaseEntity();
            }
            return NULL;
        }
    };

    class CBaseAnimating : public CBaseEntity
    {
    public:
        // dynamic_cast from CBaseEntity to CBaseAnimating
        static CBaseAnimating* CastFromBaseEntity(CBaseEntity* pEntity)
        {
            return reinterpret_cast<CBaseAnimating*>(pEntity);
        }
    };

    class CGlobalEntityList;
    class IEntityFindFilter;

    IVEngineServer* engine;
    CGlobalEntityList* gEntList;

    void(THISCALL_CONV*CBaseAnimating_DrawServerHitboxes)(CBaseAnimating* this_, float duration, bool monocolor);
    CBaseEntity*(THISCALL_CONV*CGlobalEntityList_FindEntityByName)(CGlobalEntityList* this_, CBaseEntity* pStartEntity, const char* szName, CBaseEntity* pSearchingEntity, CBaseEntity* pActivator, CBaseEntity* pCaller, IEntityFindFilter* pFilter);
};

// On plugin load
bool CShowHitboxesPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameInterfaceFactory)
{
    ConnectTier1Libraries(&interfaceFactory, 1);
    MathLib_Init();

    // Load IVEngineServer
    if ((TF2::engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL)) == NULL)
    {
        Warning("Failed to load IVEngineServer\n");
        return false;
    }
    Msg("Loaded engine\n");

    // Get server module info
    ModuleInfo serverInfo;
    if (!BGetServerModule(&serverInfo))
        return false;

    // Need CBaseAnimating::DrawServerHitboxes(...)
#ifdef _LINUX
    // linux server.so 7370160: 55 89 E5 57 56 53 83 EC 7C 8B 7D ? 0F B6 45
    TF2::CBaseAnimating_DrawServerHitboxes = (decltype(TF2::CBaseAnimating_DrawServerHitboxes))FindBytePattern(&serverInfo, "\x55\x89\xE5\x57\x56\x53\x83\xEC\x7C\x8B\x7D\x2A\x0F\xB6\x45");
#endif
#ifdef _WIN32
    TF2::CBaseAnimating_DrawServerHitboxes = (decltype(TF2::CBaseAnimating_DrawServerHitboxes))FindBytePattern(&serverInfo, "\x55\x8B\xEC\x83\xEC\x44\x57\x8B\xF9\x80\xBF\x2A\x2A\x2A\x2A\x00");
#endif
    if (!TF2::CBaseAnimating_DrawServerHitboxes)
    {
        Warning("Failed to find CBaseAnimating::DrawServerHitboxes\n");
        return false;
    }
    Msg("CBaseAnimating::DrawServerHitboxes -> 0x%lX\n", TF2::CBaseAnimating_DrawServerHitboxes);

    // Need CGlobalEntityList::FindEntityByName(...)
#ifdef _LINUX
    // linux server.so 7370160: 55 89 E5 57 56 53 83 EC 1C 8B 45 ? 8B 5D ? 8B 7D ? 8B 55
    TF2::CGlobalEntityList_FindEntityByName = (decltype(TF2::CGlobalEntityList_FindEntityByName))FindBytePattern(&serverInfo, "\x55\x89\xE5\x57\x56\x53\x83\xEC\x1C\x8B\x45\x2A\x8B\x5D\x2A\x8B\x7D\x2A\x8B\x55");
#endif
#ifdef _WIN32
    TF2::CGlobalEntityList_FindEntityByName = (decltype(TF2::CGlobalEntityList_FindEntityByName))FindBytePattern(&serverInfo, "\x55\x8B\xEC\x53\x8B\x5D\x2A\x56\x8B\xF1\x85\xDB\x74\x2A\x8A\x03");
#endif
    if (!TF2::CGlobalEntityList_FindEntityByName)
    {
        Warning("Failed to find CGlobalEntityList::FindEntityByName\n");
        return false;
    }
    Msg("CGlobalEntityList::FindEntityByName -> 0x%lX\n", TF2::CGlobalEntityList_FindEntityByName);

    // Need gEntList (global CGlobalEntityList instance)
#ifdef _LINUX
    //                                   | right here
    //                                   V
    // linux server.so 7370160: C7 04 24 ? ? ? ? E8 ? ? ? ? C7 04 24 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? A1 ? ? ? ? C7 04 24 01 00 00 00
    DWORD dwEntListPtr = FindBytePattern(&serverInfo, "\xC7\x04\x24\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xC7\x04\x24\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\xC7\x04\x24\x01\x00\x00\x00");
    if (dwEntListPtr != 0)
    {
        dwEntListPtr += 4;
        dwEntListPtr = *(DWORD*)dwEntListPtr;
    }
#endif
#ifdef _WIN32
    DWORD dwEntListPtr = FindBytePattern(&serverInfo, "\xB9\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x50\x8D\x8E\x2A\x2A\x2A\x2A");
    if (dwEntListPtr != 0)
    {
        dwEntListPtr += 1;
        dwEntListPtr = *(DWORD*)dwEntListPtr;
    }
#endif
    TF2::gEntList = (decltype(TF2::gEntList))dwEntListPtr;
    if (!TF2::gEntList)
    {
        Warning("Failed to find CGlobalEntityList instance\n");
        return false;
    }
    Msg("CGlobalEntityList instance: 0x%lX\n", TF2::gEntList);

    ConVar_Register();

    return true; // ok
}

// On plugin unload
void CShowHitboxesPlugin::Unload()
{
    ConVar_Unregister();
    DisconnectTier1Libraries();
}

// Description
const char* CShowHitboxesPlugin::GetPluginDescription()
{
    return "sv_showhitboxes reimplementation by CanteenPowered";
}


// Called on game frame
void CShowHitboxesPlugin::GameFrame(bool bSimulating)
{
    // Old implementation:
    // https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/mp/src/game/server/gameinterface.cpp#L1307
    if (!bSimulating)
        return;

    if (sv_showhitboxes.GetInt() == -1)
        return;

    if (sv_showhitboxes.GetInt() == 0)
    {
        // Find entities by name
        TF2::CBaseEntity* pEntity = NULL;
        while (1)
        {
            pEntity = TF2::CGlobalEntityList_FindEntityByName(TF2::gEntList, pEntity, sv_showhitboxes.GetString(), NULL, NULL, NULL, NULL);
            if (!pEntity)
                break;

            TF2::CBaseAnimating* pAnim = TF2::CBaseAnimating::CastFromBaseEntity(pEntity);
            if (pAnim)
                TF2::CBaseAnimating_DrawServerHitboxes(pAnim, 0.0f, false);
            }
    }
    else
    {
        // Find entity by index
        TF2::CBaseEntity* pEntity = TF2::CBaseEntity::Instance(TF2::engine->PEntityOfEntIndex(sv_showhitboxes.GetInt()));
        TF2::CBaseAnimating* pAnim = TF2::CBaseAnimating::CastFromBaseEntity(pEntity);
        if (pAnim)
            TF2::CBaseAnimating_DrawServerHitboxes(pAnim, 0.0f, false);

    }
}