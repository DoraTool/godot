/**************************************************************************/
/*  editor_version_button.cpp                                             */
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

#include "editor_version_button.h"

#include "core/os/time.h"
#include "core/version.h"
#include "editor/editor_node.h"
#include "scene/main/timer.h"

String _get_version_string(EditorVersionButton::VersionFormat p_format, bool p_easy_mode = false) {
	String main;
	switch (p_format) {
		case EditorVersionButton::FORMAT_BASIC: {
			main = VERSION_FULL_CONFIG;
		} break;
		case EditorVersionButton::FORMAT_WITH_BUILD: {
			main = "v" VERSION_FULL_BUILD;
		} break;
		case EditorVersionButton::FORMAT_WITH_NAME_AND_BUILD: {
			main = VERSION_FULL_NAME;
		} break;
		default: {
			ERR_FAIL_V_MSG(VERSION_FULL_NAME, "Unexpected format: " + itos(p_format));
		} break;
	}

	String hash = VERSION_HASH;
	if (!hash.is_empty()) {
		hash = vformat(" [%s]", hash.left(9));
	}

	// Prefix with tilde when easy mode is active.
	if (p_easy_mode) {
		if (p_format == EditorVersionButton::FORMAT_BASIC) {
			return "~" + main;
		} else {
			return "~" + main + hash;
		}
	}
	return main + hash;
}

void EditorVersionButton::_click_timer_timeout() {
	click_count = 0;
}

void EditorVersionButton::_update_version_text() {
	bool easy_mode = EditorNode::get_singleton() && EditorNode::get_singleton()->is_easy_mode();
	set_text(_get_version_string(format, easy_mode));
}

void EditorVersionButton::_on_easy_mode_changed(bool p_enabled) {
	_update_version_text();
}

void EditorVersionButton::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			// This can't be done in the constructor because theme cache is not ready yet.
			set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
			_update_version_text();

			// Connect to easy_mode_changed signal (only in editor, not ProjectManager).
			if (EditorNode::get_singleton() && EditorNode::get_singleton()->has_signal("easy_mode_changed")) {
				EditorNode::get_singleton()->connect("easy_mode_changed", callable_mp(this, &EditorVersionButton::_on_easy_mode_changed));
			}
		} break;
	}
}

void EditorVersionButton::pressed() {
	// Increment click counter for secret easy mode toggle.
	click_count++;

	if (click_timer) {
		click_timer->stop();
		click_timer->start();
	}

	if (click_count >= EASY_MODE_CLICK_COUNT) {
		click_count = 0;
		if (click_timer) {
			click_timer->stop();
		}
		// Toggle easy mode.
		if (EditorNode::get_singleton()) {
			EditorNode::get_singleton()->set_easy_mode(!EditorNode::get_singleton()->is_easy_mode());
		}
		return;
	}

	DisplayServer::get_singleton()->clipboard_set(_get_version_string(FORMAT_WITH_BUILD));
}

EditorVersionButton::EditorVersionButton(VersionFormat p_format) {
	format = p_format;
	set_underline_mode(LinkButton::UNDERLINE_MODE_ON_HOVER);

	String build_date;
	if (VERSION_TIMESTAMP > 0) {
		build_date = Time::get_singleton()->get_datetime_string_from_unix_time(VERSION_TIMESTAMP, true) + " UTC";
	} else {
		build_date = TTR("(unknown)");
	}
	set_tooltip_text(vformat(TTR("Git commit date: %s\nClick to copy the version information."), build_date));

	// Setup click timer for easy mode secret toggle (2 second timeout).
	click_timer = memnew(Timer);
	click_timer->set_one_shot(true);
	click_timer->set_wait_time(2.0);
	click_timer->connect("timeout", callable_mp(this, &EditorVersionButton::_click_timer_timeout));
	add_child(click_timer);
}
