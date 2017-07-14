#ifndef _CMD_DEF_H
#define _CMD_DEF_H 1

extern int help_cli (U32 argc, char **argv, contextInfo_t *c);
extern int quit_cli (U32 argc, char **argv, contextInfo_t *c);
extern int debug_show_cmd (U32 argc, char **argv, contextInfo_t *c);

int switch_mode_su_cmd (U32 argc, char **argv, contextInfo_t *c);
int switch_mode_user_cmd (U32 argc, char **argv, contextInfo_t *c);
int clear_screen (U32 argc, char **argv, contextInfo_t *c);
int enable_debug_cmd (U32 argc, char **argv, contextInfo_t *c);
int disable_debug_cmd (U32 argc, char **argv, contextInfo_t *c);

int get_info (U32 argc, char **argv, contextInfo_t *c);
int get_info_all (U32 argc, char **argv, contextInfo_t *c);
int get_feature_info (U32 argc, char **argv, contextInfo_t *c);
int get_general_info (U32 argc, char **argv, contextInfo_t *c);
int get_register_info (U32 argc, char **argv, contextInfo_t *c);
int get_command_info (U32 argc, char **argv, contextInfo_t *c);

int get_dma_ctx_snapshot (U32 argc, char **argv, contextInfo_t *c);

int wearing_info (U32 argc, char **argv, contextInfo_t *c);
#endif
