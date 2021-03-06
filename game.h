#ifndef _GAME_H
#define _GAME_H

#include <stdio.h>

#include "lodge.h"
#include "vfs.h"

#include "lodge_window.h"

struct graphics;
struct input;
struct frames;
struct console;
struct env;
struct core;
struct assets;

struct shared_memory
{
	void* game_memory;
	struct core* core;
	struct sound* sound;
	struct assets* assets;
	struct vfs* vfs;
	struct input* input;
};

#ifdef __cplusplus
extern "C"
{
#endif

SHARED_SYMBOL void game_init();

SHARED_SYMBOL void game_init_memory(struct shared_memory* shared_memory, int reload);

SHARED_SYMBOL void game_assets_load();
SHARED_SYMBOL void game_assets_release();

SHARED_SYMBOL void game_think(struct graphics* g, float delta_time);
SHARED_SYMBOL void game_render(struct graphics* g, float delta_time);

SHARED_SYMBOL void game_key_callback(lodge_window_t window, int key, int scancode, int action, int mods);
SHARED_SYMBOL void game_mousebutton_callback(lodge_window_t window, int button, int action, int mods);
SHARED_SYMBOL void game_fps_callback(struct frames* f);

SHARED_SYMBOL void game_console_init(struct console *c, struct env* env);

SHARED_SYMBOL struct lodge_settings* game_get_settings();

#ifdef __cplusplus
}
#endif

#endif
