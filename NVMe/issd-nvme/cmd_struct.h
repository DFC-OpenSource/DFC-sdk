#ifndef _CMD_STRUCT_H
#define _CMD_STRUCT_H

#include "cmd_defn.h"

static const argv_t wearing_info_t[] = {       
	{"wearing_info", KEYWORD, "get the wearleveling info of each page and block"},
	{"<file-name>", ARGUMENT, "File name to store the info"},
};

static const argv_t exit_t[] = {
	{"exit", KEYWORD, "Quit the Session"},
};

static const argv_t reboot_t[] = {
	{"reboot", KEYWORD, "Reboot the system"},
};

static const argv_t help_t[] = {
	{"help", KEYWORD, "HELP"},
};

#ifdef CMD_HISTORY
static const argv_t history_t[] = {
	{"history", KEYWORD,"show history"}
};
#endif

static const argv_t quit_t[] = {
	{"quit", KEYWORD, "Exit the Session"},
};

static const argv_t logout_t[] = {
	{"logout", KEYWORD, "Logout the Session"},
};

static const argv_t save_t[] = {
	{"save", KEYWORD, "Save the configuration"},
};

static const argv_t switch_mode_su_t[] = {
	{"switch", KEYWORD, "Switch command"},
	{"mode", KEYWORD, "Switch mode"},
	{"super-user", KEYWORD, "Switch to super-user mode"},
};

static const argv_t switch_mode_user_t[] = {
	{"switch", KEYWORD, "Switch user command"},
	{"mode", KEYWORD, "Switch mode"},
	{"user", KEYWORD, "Switch to user mode"},
};

static const argv_t clear_cmd[] = {       
	{"clear", KEYWORD, "Clear the screen"}
};


static const argv_t getInfo_cmd[] = {       
	{"getinfo", KEYWORD, "getinfo Command"},
};

static const argv_t getInfo_all[] = {       
	{"getinfo", KEYWORD, "getinfo Command"},
	{"all", KEYWORD, "All Info"},
};

static const argv_t getInfoCmd_cmd[] = {       
	{"getinfo", KEYWORD, "getinfo Command"},
	{"command", KEYWORD, "Command Info"},
};
static const argv_t getDmaCtxSnapshot_cmd[] = {       
	{"getinfo", KEYWORD, "get Dma Context Snapshot Command"},
	{"dma", KEYWORD, "dma info"},
	{"ctx", KEYWORD, "ctx Info"},
};

static const argv_t getInfo_feature[] = {       
	{"getinfo", KEYWORD, "getinfo Command"},
	{"feature", KEYWORD, "feature Info"},
};
static const argv_t getInfo_general[] = {       
	{"getinfo", KEYWORD, "getinfo Command"},
	{"general", KEYWORD, "General Info"},
};

static const argv_t getInfo_reg[] = {       
	{"getinfo", KEYWORD, "getinfo Command"},
	{"register", KEYWORD, "Register Info"},
};

static const argv_t clr_log_t[] = {
	{"clear", KEYWORD, "Clear Command"},
	{"log", KEYWORD, "Clear the Log Messages"},
};

static const argv_t config_date_t[] = {
	{"configure", KEYWORD, "Configure Command"},
	{"date", KEYWORD, "Date Settings"},
	{"<dd-mm-yyyy>", ARGUMENT, "Format: dd-mm-yyyy"},
};


static const argv_t config_def_gw_t[] = {
	{"configure", KEYWORD, "Configure command"},
	{"system", KEYWORD, "System settings"},
	{"gateway", KEYWORD, "Gateway settings"},
	{"default", KEYWORD, "Default gateway"},
	{"<ip-address>", ARGUMENT, "IP Address of the gateway"},
};

static const argv_t config_ipadd_t[] = {
	{"configure", KEYWORD, "Configure command"},
	{"system", KEYWORD, "System settings"},
	{"ipaddress", KEYWORD, "IP Address settings"},
	{"<ip-address>", ARGUMENT, "IP Address of the system"},
	{"netmask", KEYWORD, "Netmask settings"},
	{"<netmask>", ARGUMENT, "Netmask of the system"},
};

static const argv_t config_pwrsock_string_t[] = {
	{"configure", KEYWORD, "Configure command"},
	{"power-socket", KEYWORD, "Power-socket settings"},
	{"<port-no>", ARGUMENT, "Port number of the power-socket Range: < 1 to 4>"},
	{"display-string", KEYWORD, "Display-string settings"},
	{"<display-string>", ARGUMENT, "Name to be assigned to the port number"},
};

static const argv_t config_sport_string_t[] = {
	{"configure", KEYWORD, "Configure command"},
	{"serial-port", KEYWORD, "Serial port settings"},
	{"<port-no>", ARGUMENT, "Port number of the serial port Range: < 1 to 4>"},
	{"display-string", KEYWORD, "Display-string settings"},
	{"<display-string>", ARGUMENT, "Name to be assigned to the port number"},
};

static const argv_t config_sport_eon_t[] = {
	{"configure", KEYWORD, "Configure command"},
	{"serial-port", KEYWORD, "Serial port settings"},
	{"<port-no>", ARGUMENT, "Port number of the serial port Range: < 1 to 4>"},
	{"baud-rate", KEYWORD, "Baud rate settings"},
	{"<baud-rate>", ARGUMENT, "Baud-rate () bauds/sec"},
	{"data-bits", KEYWORD, "Data bit settings"},
	{"<no-of-data-bits>", ARGUMENT, "Number of Data bits () "},
	{"stop-bits", KEYWORD, "Stop bit settings"},
	{"<no-of-stop-bits>", ARGUMENT, "Stop bit ()"},
	{"parity", KEYWORD, "Parity settings"},
	{"even", KEYWORD, "Even parity settings"},
	{"flow-control", KEYWORD, "Flow-control settings settings"},
	{"on", KEYWORD, "Turn on flow-control"},
};

static const argv_t config_sport_oon_t[] = {
	{"configure", KEYWORD, "Configure command"},
	{"serial-port", KEYWORD, "Serial port settings"},
	{"<port-no>", ARGUMENT, "Port number of the serial port Range: < 1 to 4>"},
	{"baud-rate", KEYWORD, "Baud rate settings"},
	{"<baud-rate>", ARGUMENT, "Baud-rate () bauds/sec"},
	{"data-bits", KEYWORD, "Data bit settings"},
	{"<no-of-data-bits>", ARGUMENT, "Number of Data bits () "},
	{"stop-bits", KEYWORD, "Stop bit settings"},
	{"<no-of-stop-bits>", ARGUMENT, "Stop bit ()"},
	{"parity", KEYWORD, "Parity settings"},
	{"odd", KEYWORD, "Odd parity settings"},
	{"flow-control", KEYWORD, "Flow-control settings settings"},
	{"on", KEYWORD, "Turn on flow-control"},
};

static const argv_t config_sport_eoff_t[] = {
	{"configure", KEYWORD, "Configure command"},
	{"serial-port", KEYWORD, "Serial port settings"},
	{"<port-no>", ARGUMENT, "Port number of the serial port Range: < 1 to 4>"},
	{"baud-rate", KEYWORD, "Baud rate settings"},
	{"<baud-rate>", ARGUMENT, "Baud-rate () bauds/sec"},
	{"data-bits", KEYWORD, "Data bit settings"},
	{"<no-of-data-bits>", ARGUMENT, "Number of Data bits () "},
	{"stop-bits", KEYWORD, "Stop bit settings"},
	{"<no-of-stop-bits>", ARGUMENT, "Stop bit ()"},
	{"parity", KEYWORD, "Parity settings"},
	{"even", KEYWORD, "Even parity settings"},
	{"flow-control", KEYWORD, "Flow-control settings settings"},
	{"off", KEYWORD, "Turn off flow-control"},
};

static const argv_t config_sport_ooff_t[] = {
	{"configure", KEYWORD, "Configure command"},
	{"serial-port", KEYWORD, "Serial port settings"},
	{"<port-no>", ARGUMENT, "Port number of the serial port Range: < 1 to 4>"},
	{"baud-rate", KEYWORD, "Baud rate settings"},
	{"<baud-rate>", ARGUMENT, "Baud-rate () bauds/sec"},
	{"data-bits", KEYWORD, "Data bit settings"},
	{"<no-of-data-bits>", ARGUMENT, "Number of Data bits () "},
	{"stop-bits", KEYWORD, "Stop bit settings"},
	{"<no-of-stop-bits>", ARGUMENT, "Stop bit ()"},
	{"parity", KEYWORD, "Parity settings"},
	{"odd", KEYWORD, "Odd parity settings"},
	{"flow-control", KEYWORD, "Flow-control settings settings"},
	{"off", KEYWORD, "Turn off flow-control"},
};

static const argv_t config_time_t[] = {
	{"configure", KEYWORD, "Configure Command"},
	{"time", KEYWORD, "Time Settings"},
	{"<hh:mm>", ARGUMENT, "Format: hh:mm"},
};

static const argv_t config_uname_t[] = {
	{"configure", KEYWORD, "Configure Command"},
	{"user", KEYWORD, "User"},
	{"<username>", ARGUMENT, "User name"},
};

static const argv_t create_uname_t[] = {
	{"create", KEYWORD, "Create Command"},
	{"user", KEYWORD, "User"},
	{"<username>", ARGUMENT, "User name"},
};

static const argv_t delete_user_t[] = {
	{"delete", KEYWORD, "Delete Command"},
	{"user", KEYWORD, "User"},
	{"<username>", ARGUMENT, "User name"},
};

static const argv_t enable_port_t[] = {
	{"enable", KEYWORD, "Enable Command"},
	{"port", KEYWORD, "Port"},
};

static const argv_t enable_port_all_t[] = {
	{"enable", KEYWORD, "Enable Command"},
	{"port", KEYWORD, "Port"},
	{"all", KEYWORD, "All Ports"},
};

static const argv_t enable_port_no_t[] = {
	{"enable", KEYWORD, "Enable Command"},
	{"port", KEYWORD, "Port"},
	{"<port-no>", ARGUMENT, "Port-no"},
};

static const argv_t enable_debug_t[] = {
	{"enable", KEYWORD, "Enable Command"},
	{"debug", KEYWORD, "Debugging features"},
	{"mode", KEYWORD, "Enable debug mode"},
};

static const argv_t disable_debug_t[] = {
	{"disable", KEYWORD, "Disable Command"},
	{"debug", KEYWORD, "Debugging features"},
	{"mode", KEYWORD, "Disable debug mode"},
};

static const argv_t power_on_t[] = {
	{"power", KEYWORD, "Power Command"},
	{"on", KEYWORD, "Turn On Power"},
	{"<port-no>", ARGUMENT, "Port number: < 1 to 4>"},
};

static const argv_t power_off_t[] = {
	{"power", KEYWORD, "Power Command"},
	{"off", KEYWORD, "Turn Off Power"},
	{"<port-no>", ARGUMENT, "Port number: < 1 to 4>"},
};

static const argv_t power_cycle_t[] = {
	{"power", KEYWORD, "Power Command"},
	{"cycle", KEYWORD, "Turn Off Power and Turn On again"},
	{"<port-no>", ARGUMENT, "Port number: < 1 to 4>"},
};

static const argv_t show_date_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"date", KEYWORD, "To Display Date"},
};

static const argv_t show_time_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"time", KEYWORD, "To Display Time"},
};

static const argv_t show_log_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"log", KEYWORD, "To Display Log Messages"},
};

static const argv_t show_pwrsock_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"power-socket", KEYWORD, "Power-Socket settings"},
	{"<port-no>", ARGUMENT, "Port number: < 1 to 4>"},
};

static const argv_t show_sport_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"serial-port", KEYWORD, "Serial-Port settings"},
	{"<port-no>", ARGUMENT, "Port number: < 1 to 4>"},
};

static const argv_t show_sport_stats_port_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"serial-port", KEYWORD, "Serial-Port settings"},
	{"statistics", KEYWORD,"Statistics"},
	{"<port-no>", ARGUMENT, "Port number: < 1 to 4>"},
};

static const argv_t show_sport_stats_all_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"serial-port", KEYWORD, "Serial-Port settings"},
	{"statistics", KEYWORD,"Statistics"},
	{"all", KEYWORD, "All serial ports"},
};

static const argv_t show_stats_port_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"statistics", KEYWORD,"Statistics"},
	{"<port-no>", ARGUMENT, "Port number: < 1 to 4>"},
};

static const argv_t show_stats_all_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"statistics", KEYWORD,"Statistics"},
	{"all", KEYWORD, "For all ports"},
};

static const argv_t show_stats_mgmt_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"statistics", KEYWORD,"Statistics"},
	{"mgmt", KEYWORD, "Management Port"},
};

static const argv_t show_sys_info_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"system", KEYWORD,"System settings"},
	{"info", KEYWORD, "To display System information"},
};


static const argv_t show_user_port_t[] = {
	{"show", KEYWORD, "Show Command"},
	{"user", KEYWORD,"user"},
	//  {"<port-no>", ARGUMENT, "Port number: < 1 to 4>"},
};

static const argv_t upgrade_fw_t[] = {
	{"upgrade", KEYWORD, "Upgrade Command"},
	{"firmware", KEYWORD,"firmware"},
	{"<host-ip>", ARGUMENT, "IP Address of the host"},
	{"<file-name>", ARGUMENT, "filename"},
};

static const argv_t debug_save_t[] = {
	{"debug", KEYWORD, "Debug Command"},
	{"save", KEYWORD,"Save"},
	{"fru", KEYWORD, "FRU"},
};

static const argv_t debug_show_fru_t[] = {
	{"debug", KEYWORD, "Debug Command"},
	{"show", KEYWORD,"Show"},
	{"fru", KEYWORD, "FRU"},
};

static const argv_t debug_show_cmd_t[] = {
	{"debug", KEYWORD, "Debug Command"},
	{"show", KEYWORD, "Show"},
	{"commands", KEYWORD, "List all available commands"},
};

static const argv_t debug_show_log_t[] = {
	{"debug", KEYWORD, "Debug Command"},
	{"show", KEYWORD,"Show"},
	{"log", KEYWORD, "Log settings"},
	{"level", KEYWORD, "To dispaly Log Level"},
};

static const argv_t debug_config_mac_t[] = {
	{"debug", KEYWORD, "Debug Command"},
	{"configure", KEYWORD,"Configure Command"},
	{"mac-address", KEYWORD, "MAC Address settings"},
	{"<mac-address>", ARGUMENT, "MAC Address of the system"},
};

static const argv_t debug_config_snumber_t[] = {
	{"debug", KEYWORD, "Debug Command"},
	{"configure", KEYWORD,"Configure Command"},
	{"product-number", KEYWORD, "Product Number settings"},
	{"<serial-number>", ARGUMENT, "Serial Number of the system"},
};

static const argv_t debug_config_log_t[] = {
	{"debug", KEYWORD, "Debug Command"},
	{"configure", KEYWORD,"Configure Command"},
	{"log", KEYWORD, "Log settings"},
	{"level", KEYWORD, "To dispaly Log Level"},
	{"<log-level", ARGUMENT, "Log Level Bit-mask: EMERG(4), ERR(3), WARN(2), INFO(1), DEBUG(0)"},
};

static const argv_t debug_config_log_on_t[] = {
	{"debug", KEYWORD, "Debug Command"},
	{"configure", KEYWORD,"Configure Command"},
	{"log", KEYWORD, "Log settings"},
	{"display", KEYWORD, "Dispaly"},
	{"on", KEYWORD, "Turn on log Messages"},
};

static const argv_t debug_config_log_off_t[] = {
	{"debug", KEYWORD, "Debug Command"},
	{"configure", KEYWORD,"Configure Command"},
	{"log", KEYWORD, "Log settings"},
	{"display", KEYWORD, "Dispaly"},
	{"off", KEYWORD, "Turn off log Messages"},
};

/* NOTE: The commands are to arranged in ASCENDING order with each
 * domain, based on the number of WORDS (NOT letters) in each
 * command. Also to be grouped into various sub-sets, following the
 * same ascending order within the sub-sets too. */

/* Eg: show version      */
/*     show poe          */
/*     show poe status   */
/*     show poe detail   */
/*     show port         */
/*     show port details */
/*     show port stats   */
/*     show port name    */
/*     show port number  */
/*     show port status  */

static const cmd_struct_t haystack[] = {


	{get_info, SIZEOF(getInfo_cmd), (argv_t *)getInfo_cmd, NORMAL_COMMAND},

	{get_info_all, SIZEOF(getInfo_all), (argv_t *)getInfo_all, NORMAL_COMMAND},

	{get_feature_info, SIZEOF(getInfo_feature), (argv_t *)getInfo_feature, NORMAL_COMMAND},

	{get_general_info, SIZEOF(getInfo_general), (argv_t *)getInfo_general, NORMAL_COMMAND},

	{get_register_info, SIZEOF(getInfo_reg), (argv_t *)getInfo_reg, NORMAL_COMMAND},

	{get_command_info, SIZEOF(getInfoCmd_cmd), (argv_t *)getInfoCmd_cmd, NORMAL_COMMAND},

	{get_dma_ctx_snapshot, SIZEOF(getDmaCtxSnapshot_cmd), (argv_t *)getDmaCtxSnapshot_cmd, NORMAL_COMMAND},

	{quit_cli, SIZEOF(exit_t), (argv_t *)exit_t, NORMAL_COMMAND},

	{NULL, SIZEOF(reboot_t), (argv_t *)reboot_t, ADMIN_COMMAND},

	{help_cli, SIZEOF(help_t), (argv_t *)help_t, NORMAL_COMMAND},

#ifdef CMD_HISTORY
	{print_history, SIZEOF(history_t), (argv_t *)history_t, NORMAL_COMMAND},
#endif

	{quit_cli, SIZEOF(quit_t), (argv_t *)quit_t, NORMAL_COMMAND},

	{NULL, SIZEOF(save_t), (argv_t *)save_t, ADMIN_COMMAND},

	{switch_mode_su_cmd, SIZEOF(switch_mode_su_t), (argv_t *)switch_mode_su_t, NORMAL_COMMAND},

	{switch_mode_user_cmd, SIZEOF(switch_mode_user_t), (argv_t *)switch_mode_user_t, NORMAL_COMMAND},

	{clear_screen, SIZEOF(clear_cmd), (argv_t *)clear_cmd, NORMAL_COMMAND},

	{wearing_info, SIZEOF(wearing_info_t), (argv_t *)wearing_info_t, NORMAL_COMMAND},

	{NULL, SIZEOF(clr_log_t), (argv_t *)clr_log_t, NORMAL_COMMAND},

	{NULL, SIZEOF(config_date_t), (argv_t *)config_date_t, ADMIN_COMMAND},

	{NULL, SIZEOF(config_def_gw_t), (argv_t *)config_def_gw_t, ADMIN_COMMAND},

	{NULL, SIZEOF(config_ipadd_t), (argv_t *)config_ipadd_t, ADMIN_COMMAND},

	{NULL, SIZEOF(config_pwrsock_string_t), (argv_t *)config_pwrsock_string_t, NORMAL_COMMAND},

	{NULL, SIZEOF(config_sport_string_t), (argv_t *)config_sport_string_t, NORMAL_COMMAND},

	{NULL, SIZEOF(config_sport_eon_t), (argv_t *)config_sport_eon_t, ADMIN_COMMAND},

	{NULL, SIZEOF(config_sport_eoff_t), (argv_t *)config_sport_eoff_t, ADMIN_COMMAND},

	{NULL, SIZEOF(config_sport_oon_t), (argv_t *)config_sport_oon_t, ADMIN_COMMAND},

	{NULL, SIZEOF(config_sport_ooff_t), (argv_t *)config_sport_ooff_t, ADMIN_COMMAND},

	{NULL, SIZEOF(config_time_t), (argv_t *)config_time_t, ADMIN_COMMAND},

	{NULL, SIZEOF(config_uname_t), (argv_t *)config_uname_t, ADMIN_COMMAND},

	{NULL, SIZEOF(create_uname_t), (argv_t *)create_uname_t, ADMIN_COMMAND},

	{NULL, SIZEOF(delete_user_t), (argv_t *)delete_user_t, ADMIN_COMMAND},

	{NULL, SIZEOF(enable_port_t), (argv_t *)enable_port_t, NORMAL_COMMAND},

	{NULL, SIZEOF(enable_port_all_t), (argv_t *)enable_port_all_t, NORMAL_COMMAND},

	{NULL, SIZEOF(enable_port_no_t), (argv_t *)enable_port_no_t, NORMAL_COMMAND},

	{enable_debug_cmd, SIZEOF(enable_debug_t), (argv_t *)enable_debug_t, HIDDEN_COMMAND | ADMIN_COMMAND},

	{disable_debug_cmd, SIZEOF(disable_debug_t), (argv_t *)disable_debug_t, HIDDEN_COMMAND | ADMIN_COMMAND},

	{NULL, SIZEOF(power_on_t), (argv_t *)power_on_t, NORMAL_COMMAND},

	{NULL, SIZEOF(power_off_t), (argv_t *)power_off_t, NORMAL_COMMAND},

	{NULL, SIZEOF(power_cycle_t), (argv_t *)power_cycle_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_date_t), (argv_t *)show_date_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_time_t), (argv_t *)show_time_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_log_t), (argv_t *)show_log_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_pwrsock_t), (argv_t *)show_pwrsock_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_sport_t), (argv_t *)show_sport_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_sport_stats_port_t), (argv_t *)show_sport_stats_port_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_sport_stats_all_t), (argv_t *)show_sport_stats_all_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_stats_port_t), (argv_t *)show_stats_port_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_stats_all_t), (argv_t *)show_stats_all_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_stats_mgmt_t), (argv_t *)show_stats_mgmt_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_sys_info_t), (argv_t *)show_sys_info_t, NORMAL_COMMAND},

	{NULL, SIZEOF(show_user_port_t), (argv_t *)show_user_port_t, NORMAL_COMMAND},

	{NULL, SIZEOF(upgrade_fw_t), (argv_t *)upgrade_fw_t, ADMIN_COMMAND},

	{NULL, SIZEOF(debug_save_t), (argv_t *)debug_save_t, DEBUG_COMMAND},

	{debug_show_cmd, SIZEOF(debug_show_cmd_t), (argv_t *)debug_show_cmd_t, DEBUG_COMMAND},

	{NULL, SIZEOF(debug_show_fru_t), (argv_t *)debug_show_fru_t, DEBUG_COMMAND},

	{NULL, SIZEOF(debug_show_log_t), (argv_t *)debug_show_log_t, DEBUG_COMMAND},

	{NULL, SIZEOF(debug_config_mac_t), (argv_t *)debug_config_mac_t, DEBUG_COMMAND},

	{NULL, SIZEOF(debug_config_snumber_t), (argv_t *)debug_config_snumber_t, DEBUG_COMMAND},

	{NULL, SIZEOF(debug_config_log_t), (argv_t *)debug_config_log_t, DEBUG_COMMAND},

	{NULL, SIZEOF(debug_config_log_on_t), (argv_t *)debug_config_log_on_t, DEBUG_COMMAND},

	{NULL, SIZEOF(debug_config_log_off_t), (argv_t *)debug_config_log_off_t, DEBUG_COMMAND},

};
#endif
