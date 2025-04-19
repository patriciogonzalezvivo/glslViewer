#pragma once

#include <string>
#include "../tools/command.h"
#include "../glslViewer.h"

void console_init(int _osc_port);

bool console_is_init();

void console_clear();
void console_refresh();

void console_sigwinch_handler(int signal);

bool console_getline(std::string& _cmd, CommandList& _commands, GlslViewer& _sandbox);
void console_draw_pct(float _pct);
void console_uniforms( bool _show );
void console_uniforms_refresh();

void console_end();

void captureMouse(bool _enable);