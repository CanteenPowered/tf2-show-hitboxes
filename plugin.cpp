//
// Example Team Fortress 2 client/server plugin
//

#include "engine/iserverplugin.h"
#include "tier1/tier1.h"

//
// Example plugin class
// See: public/engine/iserverplugin.h
//
class CExamplePlugin : public IServerPluginCallbacks
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
    virtual void            GameFrame(bool) { }
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
CExamplePlugin g_Plugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CExamplePlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_Plugin);

// Example ConCommand
CON_COMMAND(example_command, "Example ConCommand")
{
    ConVarRef ref_example_convar("example_convar");
    Msg("This is an example command\n");
    Msg("The value of example_convar is %s\n", ref_example_convar.GetString());
}

// Example ConVar
ConVar example_convar("example_convar", "0", FCVAR_NONE, "An example ConVar");

// On plugin load
bool CExamplePlugin::Load(CreateInterfaceFn ifac, CreateInterfaceFn gfac)
{
    ConnectTier1Libraries(&ifac, 1);
    MathLib_Init();
    ConVar_Register();

    Msg("Hello!\n");

    return true; // ok
}

// On plugin unload
void CExamplePlugin::Unload()
{
    ConVar_Unregister();
    DisconnectTier1Libraries();
}

// Description
const char* CExamplePlugin::GetPluginDescription()
{
    return "Example Plugin by CanteenPowered";
}
