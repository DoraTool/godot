/**************************************************************************/
/*  scene_only_dock.cpp                                                  */
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

#include "scene_only_dock.h"

#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/os/os.h"
#include "core/string/translation.h"
#include "editor/editor_file_system.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/progress_bar.h"
#include "servers/display_server.h"

SceneOnlyDock::SceneOnlyDock() {
	set_name(TTR("Scenes"));

	HBoxContainer *toolbar_hb = memnew(HBoxContainer);
	add_child(toolbar_hb);

	search_box = memnew(LineEdit);
	search_box->set_h_size_flags(SIZE_EXPAND_FILL);
	search_box->set_placeholder(TTR("Search Scenes"));
	search_box->set_clear_button_enabled(true);
	toolbar_hb->add_child(search_box);

	reload_button = memnew(Button);
	reload_button->set_focus_mode(FOCUS_NONE);
	reload_button->set_tooltip_text(TTR("Re-Scan Project"));
	reload_button->connect(SceneStringName(pressed), callable_mp(this, &SceneOnlyDock::_rescan));
	toolbar_hb->add_child(reload_button);

	scene_tree = memnew(Tree);
	scene_tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	scene_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	scene_tree->set_select_mode(Tree::SELECT_MULTI);
	scene_tree->set_allow_rmb_select(true);
	scene_tree->set_hide_root(true);
	scene_tree->set_custom_minimum_size(Size2(0, 15 * EDSCALE));
	add_child(scene_tree);

	context_menu = memnew(PopupMenu);
	context_menu->add_item(TTR("Open"), CONTEXT_OPEN);
	context_menu->add_item(TTR("Instantiate"), CONTEXT_INSTANTIATE);
	context_menu->add_separator();
	context_menu->add_item(TTR("Set as Main Scene"), CONTEXT_SET_MAIN);
	context_menu->add_separator();
	context_menu->add_item(TTR("Show in File Manager"), CONTEXT_SHOW_IN_FILESYSTEM);
	context_menu->add_item(TTR("Copy Path"), CONTEXT_COPY_PATH);
	add_child(context_menu);

	scanning_vb = memnew(VBoxContainer);
	scanning_vb->set_v_size_flags(SIZE_EXPAND_FILL);
	scanning_vb->hide();
	add_child(scanning_vb);

	Label *scan_label = memnew(Label);
	scan_label->set_text(TTR("Scanning Scenes,\nPlease Wait..."));
	scan_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	scanning_vb->add_child(scan_label);

	scanning_progress = memnew(ProgressBar);
	scanning_vb->add_child(scanning_progress);

	set_process(false);
}

void SceneOnlyDock::_bind_methods() {
	ADD_SIGNAL(MethodInfo("instantiate", PropertyInfo(Variant::PACKED_STRING_ARRAY, "files")));
}

void SceneOnlyDock::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (initialized) {
				break;
			}
			initialized = true;

			context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &SceneOnlyDock::_context_menu_id_pressed));
			search_box->connect(SceneStringName(text_changed), callable_mp(this, &SceneOnlyDock::_search_changed));
			scene_tree->connect("item_activated", callable_mp(this, &SceneOnlyDock::_tree_item_activated));
			scene_tree->connect("item_mouse_selected", callable_mp(this, &SceneOnlyDock::_tree_item_rmb_selected));
			scene_tree->connect("empty_clicked", callable_mp(this, &SceneOnlyDock::_tree_empty_clicked));

			EditorFileSystem::get_singleton()->connect("filesystem_changed", callable_mp(this, &SceneOnlyDock::_filesystem_changed));

			if (EditorFileSystem::get_singleton()->is_scanning()) {
				_set_scanning_mode();
			} else {
				_filesystem_changed();
			}
		} break;
		case NOTIFICATION_PROCESS: {
			if (scanning_vb->is_visible_in_tree()) {
				scanning_progress->set_value(EditorFileSystem::get_singleton()->get_scanning_progress() * 100.0f);
			}
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			reload_button->set_button_icon(get_theme_icon(SNAME("Reload"), EditorStringName(EditorIcons)));
			search_box->set_right_icon(get_theme_icon(SNAME("Search"), EditorStringName(EditorIcons)));
			_update_tree();
		} break;
	}
}

bool SceneOnlyDock::_dir_has_scenes(EditorFileSystemDirectory *p_dir) {
	if (!p_dir) {
		return false;
	}

	// Check files in this directory.
	for (int i = 0; i < p_dir->get_file_count(); i++) {
		if (p_dir->get_file_path(i).get_extension() == "tscn") {
			return true;
		}
	}

	// Check subdirectories.
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		if (_dir_has_scenes(p_dir->get_subdir(i))) {
			return true;
		}
	}

	return false;
}

bool SceneOnlyDock::_matches_all_search_tokens(const String &p_text) const {
	if (searched_tokens.is_empty()) {
		return true;
	}

	const String check = p_text.to_lower();
	for (int i = 0; i < searched_tokens.size(); i++) {
		if (!check.contains(searched_tokens[i])) {
			return false;
		}
	}

	return true;
}

bool SceneOnlyDock::_passes_filter(const String &p_name, const String &p_path) const {
	if (searched_tokens.is_empty()) {
		return true;
	}
	return _matches_all_search_tokens(p_name) || _matches_all_search_tokens(p_path);
}

Ref<Texture2D> SceneOnlyDock::_get_scene_icon(bool p_is_valid, const String &p_file_type, const String &p_icon_path) {
	if (!p_icon_path.is_empty()) {
		Ref<Texture2D> icon = ResourceLoader::load(p_icon_path);
		if (icon.is_valid()) {
			return icon;
		}
	}

	if (!p_is_valid) {
		return get_theme_icon(SNAME("ImportFail"), EditorStringName(EditorIcons));
	} else if (has_theme_icon(p_file_type, EditorStringName(EditorIcons))) {
		return get_theme_icon(p_file_type, EditorStringName(EditorIcons));
	} else {
		return get_theme_icon(SNAME("PackedScene"), EditorStringName(EditorIcons));
	}
}

TreeItem *SceneOnlyDock::_create_tree(TreeItem *p_parent, EditorFileSystemDirectory *p_dir) {
	if (!p_dir) {
		return nullptr;
	}

	// Skip directories that have no scenes anywhere in their subtree.
	if (!_dir_has_scenes(p_dir)) {
		return nullptr;
	}

	// Create folder item.
	TreeItem *dir_item = scene_tree->create_item(p_parent);
	String dir_name = p_dir->get_name();
	if (dir_name.is_empty()) {
		dir_name = "res://";
	}
	dir_item->set_text(0, dir_name);
	dir_item->set_icon(0, get_theme_icon(SNAME("Folder"), EditorStringName(EditorIcons)));
	dir_item->set_icon_modulate(0, get_theme_color(SNAME("folder_icon_color"), SNAME("FileDialog")));
	dir_item->set_metadata(0, p_dir->get_path());
	dir_item->set_selectable(0, false);

	const String main_scene = ResourceUID::ensure_path(GLOBAL_GET("application/run/main_scene"));
	bool has_visible_children = false;

	// Add scene files.
	for (int i = 0; i < p_dir->get_file_count(); i++) {
		const String path = p_dir->get_file_path(i);
		if (path.get_extension() != "tscn") {
			continue;
		}

		const String name = p_dir->get_file(i);
		if (!_passes_filter(name, path)) {
			continue;
		}

		has_visible_children = true;

		const String file_type = p_dir->get_file_type(i);
		const String icon_path = p_dir->get_file_icon_path(i);
		const bool import_valid = p_dir->get_file_import_is_valid(i);

		TreeItem *file_item = scene_tree->create_item(dir_item);
		file_item->set_text(0, name);
		file_item->set_icon(0, _get_scene_icon(import_valid, file_type, icon_path));
		file_item->set_metadata(0, path);

		String tooltip = path;
		if (!import_valid) {
			tooltip += "\n" + TTR("Status: Import of file failed. Please fix file and reimport manually.");
		}
		file_item->set_tooltip_text(0, tooltip);

		if (path == main_scene) {
			file_item->set_custom_color(0, get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
		}
	}

	// Recurse into subdirectories.
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		TreeItem *subdir_item = _create_tree(dir_item, p_dir->get_subdir(i));
		if (subdir_item) {
			has_visible_children = true;
		}
	}

	// If filtering is active and no children passed, remove this folder.
	if (!searched_tokens.is_empty() && !has_visible_children) {
		dir_item->get_parent()->remove_child(dir_item);
		memdelete(dir_item);
		return nullptr;
	}

	return dir_item;
}

void SceneOnlyDock::_update_tree() {
	if (!initialized) {
		return;
	}

	scene_tree->clear();
	TreeItem *root = scene_tree->create_item();

	EditorFileSystemDirectory *fs_root = EditorFileSystem::get_singleton()->get_filesystem();
	if (fs_root) {
		_create_tree(root, fs_root);
	}
}

void SceneOnlyDock::_search_changed(const String &p_text) {
	searched_tokens.clear();
	const String lowercase = p_text.to_lower();
	if (!lowercase.is_empty()) {
		searched_tokens = lowercase.split(" ", false);
	}
	_update_tree();
}

void SceneOnlyDock::_tree_item_activated() {
	TreeItem *selected = scene_tree->get_selected();
	if (!selected) {
		return;
	}

	String path = selected->get_metadata(0);
	if (path.ends_with("/")) {
		// It's a folder, toggle collapse.
		selected->set_collapsed(!selected->is_collapsed());
	} else {
		_open_in_editor(path);
	}
}

void SceneOnlyDock::_tree_item_rmb_selected(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	TreeItem *selected = scene_tree->get_selected();
	if (!selected) {
		return;
	}

	String path = selected->get_metadata(0);
	if (path.ends_with("/")) {
		// Folder, don't show context menu.
		return;
	}

	context_menu->set_position(scene_tree->get_screen_position() + p_pos);
	context_menu->popup();
}

void SceneOnlyDock::_tree_empty_clicked(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}
	scene_tree->deselect_all();
}

void SceneOnlyDock::_context_menu_id_pressed(int p_id) {
	TreeItem *selected = scene_tree->get_next_selected(nullptr);
	if (!selected) {
		return;
	}

	// Collect all selected scene paths.
	PackedStringArray paths;
	while (selected) {
		String path = selected->get_metadata(0);
		if (!path.ends_with("/")) {
			paths.push_back(path);
		}
		selected = scene_tree->get_next_selected(selected);
	}

	if (paths.is_empty()) {
		return;
	}

	switch (p_id) {
		case CONTEXT_OPEN: {
			for (int i = 0; i < paths.size(); i++) {
				_open_in_editor(paths[i]);
			}
		} break;
		case CONTEXT_INSTANTIATE: {
			_instantiate_scenes(paths);
		} break;
		case CONTEXT_SET_MAIN: {
			_set_as_main_scene(paths[0]);
		} break;
		case CONTEXT_SHOW_IN_FILESYSTEM: {
			_show_in_filesystem(paths[0]);
		} break;
		case CONTEXT_COPY_PATH: {
			_copy_path(paths[0]);
		} break;
	}
}

void SceneOnlyDock::_open_in_editor(const String &p_path) {
	EditorInterface::get_singleton()->set_main_screen_editor("2D");
	EditorNode::get_singleton()->open_request(p_path);
}

void SceneOnlyDock::_instantiate_scenes(const PackedStringArray &p_paths) {
	if (p_paths.is_empty()) {
		return;
	}

	emit_signal(SNAME("instantiate"), p_paths);
}

void SceneOnlyDock::_set_as_main_scene(const String &p_path) {
	ProjectSettings::get_singleton()->set("application/run/main_scene", ResourceUID::path_to_uid(p_path));
	ProjectSettings::get_singleton()->save();
	_update_tree();
}

void SceneOnlyDock::_show_in_filesystem(const String &p_path) {
	const String abs_path = ProjectSettings::get_singleton()->globalize_path(p_path);
	OS::get_singleton()->shell_show_in_file_manager(abs_path, true);
}

void SceneOnlyDock::_copy_path(const String &p_path) {
	DisplayServer::get_singleton()->clipboard_set(p_path);
}

void SceneOnlyDock::_filesystem_changed() {
	reload_button->show();
	scanning_vb->hide();
	scene_tree->show();
	set_process(false);

	_update_tree();
}

void SceneOnlyDock::_set_scanning_mode() {
	scene_tree->hide();
	scanning_vb->show();
	scanning_progress->set_value(0);
	set_process(true);
}

void SceneOnlyDock::_rescan() {
	_set_scanning_mode();
	EditorFileSystem::get_singleton()->scan();
}
