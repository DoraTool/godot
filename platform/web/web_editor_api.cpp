/**************************************************************************/
/*  web_editor_api.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifdef TOOLS_ENABLED

#include "editor/debugger/editor_debugger_node.h"
#include "editor/editor_file_system.h"
#include "editor/editor_node.h"

#include <emscripten.h>

extern "C" {

// Start the editor's debug server on the specified channel.
// This allows external game instances to connect for live debugging.
// Returns 0 on success, non-zero on error.
EMSCRIPTEN_KEEPALIVE int godot_js_editor_start_debug_server(const char *p_channel) {
	EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
	if (!debugger) {
		return 1; // Editor not initialized
	}

	String channel = p_channel ? String::utf8(p_channel) : "default";
	String uri = "web://" + channel;

	debugger->set_keep_open(true);
	Error err = debugger->start(uri);
	return err == OK ? 0 : 2;
}

// Stop the editor's debug server.
EMSCRIPTEN_KEEPALIVE void godot_js_editor_stop_debug_server() {
	EditorDebuggerNode *debugger = EditorDebuggerNode::get_singleton();
	if (debugger) {
		debugger->stop(true);
	}
}

// Trigger a filesystem scan to reload resources from disk (e.g., after external edits).
// Returns 0 on success, non-zero on error.
EMSCRIPTEN_KEEPALIVE int godot_js_editor_scan_filesystem() {
	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (!efs) {
		return 1; // Editor not initialized
	}

	efs->scan();
	return 0;
}

// Reload modified scenes that have been changed on disk.
// Returns 0 on success, non-zero on error.
EMSCRIPTEN_KEEPALIVE int godot_js_editor_reload_modified_scenes() {
	EditorNode *editor = EditorNode::get_singleton();
	if (!editor) {
		return 1; // Editor not initialized
	}

	editor->reload_modified_scenes();
	return 0;
}

// Reload project settings from disk.
// Returns 0 on success, non-zero on error.
EMSCRIPTEN_KEEPALIVE int godot_js_editor_reload_project_settings() {
	EditorNode *editor = EditorNode::get_singleton();
	if (!editor) {
		return 1; // Editor not initialized
	}

	editor->reload_project_settings();
	return 0;
}

// Scan filesystem for changes to detect modified files.
// Returns 0 on success, non-zero on error.
EMSCRIPTEN_KEEPALIVE int godot_js_editor_scan_filesystem_changes() {
	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (!efs) {
		return 1; // Editor not initialized
	}

	efs->scan_changes();
	return 0;
}

} // extern "C"

#endif // TOOLS_ENABLED
