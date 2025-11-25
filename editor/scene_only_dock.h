/**************************************************************************/
/*  scene_only_dock.h                                                    */
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

#ifndef SCENE_ONLY_DOCK_H
#define SCENE_ONLY_DOCK_H

#include "core/input/input_event.h"
#include "core/templates/hash_set.h"
#include "scene/gui/box_container.h"
#include "scene/gui/tree.h"
#include "scene/resources/texture.h"

class Button;
class Label;
class LineEdit;
class EditorFileSystemDirectory;
class PopupMenu;
class ProgressBar;

class SceneOnlyDock : public VBoxContainer {
	GDCLASS(SceneOnlyDock, VBoxContainer);

	enum ContextMenuOption {
		CONTEXT_OPEN,
		CONTEXT_INSTANTIATE,
		CONTEXT_SET_MAIN,
		CONTEXT_SHOW_IN_FILESYSTEM,
		CONTEXT_COPY_PATH,
	};

	LineEdit *search_box = nullptr;
	Button *reload_button = nullptr;
	Tree *scene_tree = nullptr;
	PopupMenu *context_menu = nullptr;
	VBoxContainer *scanning_vb = nullptr;
	ProgressBar *scanning_progress = nullptr;

	PackedStringArray searched_tokens;
	HashSet<String> folders_with_scenes; // Folders that contain .tscn (directly or in subdirs)

	bool initialized = false;

	// Returns true if this directory or any subdirectory contains a .tscn file.
	bool _dir_has_scenes(EditorFileSystemDirectory *p_dir);
	// Recursively build tree, returns the created TreeItem for p_dir (or nullptr if skipped).
	TreeItem *_create_tree(TreeItem *p_parent, EditorFileSystemDirectory *p_dir);
	void _update_tree();
	bool _matches_all_search_tokens(const String &p_text) const;
	bool _passes_filter(const String &p_name, const String &p_path) const;
	Ref<Texture2D> _get_scene_icon(bool p_is_valid, const String &p_file_type, const String &p_icon_path);

	void _search_changed(const String &p_text);
	void _tree_item_activated();
	void _tree_item_rmb_selected(const Vector2 &p_pos, MouseButton p_button);
	void _tree_empty_clicked(const Vector2 &p_pos, MouseButton p_button);
	void _context_menu_id_pressed(int p_id);
	void _open_in_editor(const String &p_path);
	void _instantiate_scenes(const PackedStringArray &p_paths);
	void _set_as_main_scene(const String &p_path);
	void _show_in_filesystem(const String &p_path);
	void _copy_path(const String &p_path);

	void _filesystem_changed();
	void _set_scanning_mode();
	void _rescan();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	SceneOnlyDock();
};

#endif // SCENE_ONLY_DOCK_H
