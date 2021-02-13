#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

#define SMEXT_CONF_NAME			"[L4D2] Survivor Object Collection"
#define SMEXT_CONF_DESCRIPTION	"Improves behavior of survivor bots when collecting objects in environment"
#define SMEXT_CONF_VERSION		"1.0.0"
#define SMEXT_CONF_AUTHOR		"Jay"
#define SMEXT_CONF_URL			"http://ugaming.org"
#define SMEXT_CONF_LOGTAG		"L4D2-SOC"
#define SMEXT_CONF_LICENSE		"MIT"
#define SMEXT_CONF_DATESTRING	__DATE__

#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

#define SMEXT_CONF_METAMOD		

#define SMEXT_ENABLE_GAMECONF
#define SMEXT_ENABLE_GAMEHELPERS

#endif // _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_