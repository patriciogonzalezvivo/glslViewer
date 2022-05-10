#pragma once

#include <string>
#include "../types/command.h"
#include "../sandbox.h"

void console_init(int _osc_port);

void console_clear();
void console_refresh();

void console_sigwinch_handler(int signal);

bool console_getline(std::string& _cmd, CommandList& _commands, Sandbox& _sandbox);
void console_draw_pct(float _pct);
void console_uniforms( bool _show );
void console_uniforms_refresh();

void console_end();
