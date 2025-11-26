# Godot Engine AI Agent Instructions

## Architecture Overview

Godot is a C++ game engine with a custom build system (SCons) and reflection system. The codebase follows a modular architecture:

- **core/** - Engine fundamentals (Object system, Variant types, math, OS abstraction)
- **scene/** - Scene graph nodes (2D/3D nodes, UI controls, resources)
- **servers/** - Low-level rendering, physics, audio backends
- **editor/** - Built-in editor (plugins, docks, inspectors, dialogs)
- **modules/** - Optional features (gdscript, mono, gltf, etc.)
- **platform/** - OS-specific implementations (windows, linux, macos, web, mobile)
- **drivers/** - Graphics/audio driver implementations (vulkan, gles3, metal, d3d12)

## Build System & Commands

**SCons** is used for building. Key commands:
```bash
# Web editor build (this repo's focus)
scons platform=web target=editor javascript_eval=yes -j8

# Desktop editor builds
scons platform=linuxbsd target=editor -j8
scons platform=macos target=editor arch=arm64 -j8
scons platform=windows target=editor -j8

# Template builds (export runtime)
scons platform=web target=template_release -j8

# Clean builds
scons --clean
```

**Build configuration**: Edit `custom.py` at repo root for persistent build options. Use `scons -h` to see all options.

**Web editor specifics**: This fork includes custom web export APIs. See `godot.d.ts` for TypeScript definitions. Use `serve.js` (Node.js) to run the web editor locally with proper COEP/COOP headers.

## Core Object System Patterns

### GDCLASS Macro
All engine classes use `GDCLASS(ClassName, BaseClass)` for reflection:
```cpp
class MyNode : public Node2D {
    GDCLASS(MyNode, Node2D);
    
protected:
    static void _bind_methods();
    void _notification(int p_what);
    
public:
    MyNode();
};
```

### Method Binding
Expose methods to GDScript/editor via `_bind_methods()`:
```cpp
void MyNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("my_method", "param1", "param2"), &MyNode::my_method);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "my_property"), "set_my_property", "get_my_property");
    ADD_SIGNAL(MethodInfo("my_signal", PropertyInfo(Variant::STRING, "data")));
}
```

### Notifications
Lifecycle events use `_notification(int p_what)`:
- `NOTIFICATION_READY` - After scene tree initialization
- `NOTIFICATION_ENTER_TREE` - When added to scene tree
- `NOTIFICATION_EXIT_TREE` - When removed from scene tree
- `NOTIFICATION_PROCESS` - Per-frame update (if `set_process(true)`)

### Memory Management
- Use `memnew()` / `memdelete()` for raw pointers
- Use `Ref<T>` for reference-counted objects (automatically handles cleanup)
- Editor nodes typically use `memnew()` and are freed via scene tree

## Editor Plugin Architecture

Editor extends via EditorPlugin subclasses. Key patterns:

### Plugin Registration
```cpp
// In editor_node.cpp constructor:
add_editor_plugin(memnew(MyEditorPlugin));

// Dock registration:
add_control_to_dock(DOCK_SLOT_LEFT_UL, my_dock);
```

### Editor Docks
Custom docks inherit from Control/VBoxContainer and register via EditorPlugin:
```cpp
class MyDock : public VBoxContainer {
    GDCLASS(MyDock, VBoxContainer);
    
    void _notification(int p_what) {
        if (p_what == NOTIFICATION_READY) {
            // Connect to EditorNode signals
            EditorNode::get_singleton()->connect("signal_name", callable_mp(this, &MyDock::_on_signal));
        }
    }
};
```

### EditorSettings Integration
Use `EDITOR_GET()` macro and register in `editor_settings.cpp`:
```cpp
// Registration in editor_settings.cpp
_initial_set("interface/editor/my_setting", false, true);  // true = hidden setting

// Usage
if (EDITOR_GET("interface/editor/my_setting")) {
    // ...
}
```

### Timing Issues with Singletons
EditorNode and other singletons may not be fully initialized in constructors. Use `NOTIFICATION_READY` or signal connections to apply initial state:
```cpp
// BAD - singleton might not be ready
MyDock::MyDock() {
    if (EditorNode::get_singleton()->some_method()) { }  // May crash or fail
}

// GOOD - wait for ready
void MyDock::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        EditorNode::get_singleton()->connect("my_signal", callable_mp(this, &MyDock::handler));
        handler(EditorNode::get_singleton()->get_initial_state());  // Apply state
    }
}
```

## Code Style

Follow `.clang-format` strictly. Key conventions:
- Tabs for indentation
- Opening braces on same line for methods: `void method() {`
- Member variables prefixed with nothing (use descriptive names)
- Private/protected methods often prefixed with `_`
- Constants: `UPPER_SNAKE_CASE`
- Enums: `EnumName` with `ENUM_VALUE` entries
- Prefer `nullptr` over `NULL`

## Include Paths

Headers use project-relative paths from repo root:
```cpp
#include "editor/editor_node.h"          // NOT "../editor_node.h"
#include "scene/gui/button.h"
#include "core/input/input_event.h"
```

Forward declarations preferred in headers when possible.

## Common Pitfalls

1. **PackedStringArray** is NOT a header file - it's a typedef. Include `core/variant/variant.h` for variant types.

2. **Mouse button enums** use `MouseButton::LEFT`, `MouseButton::RIGHT` (not `MOUSE_BUTTON_LEFT`).

3. **Theme access timing**: In constructors, theme may not be ready. Use `NOTIFICATION_POSTINITIALIZE` or `NOTIFICATION_READY` for theme-dependent setup.

4. **EDSCALE**: Automatically adjusts to editor DPI scale. Multiply UI sizes by it: `4 * EDSCALE`.

5. **Signal connections**: Use `callable_mp(this, &ClassName::method)` for member function callbacks.

6. **Editor theme icons**: `get_editor_theme_icon(SNAME("IconName"))` with StringName for performance.

## Testing & Debugging

- Build with `target=editor` for debug symbols
- Use `print_line()`, `print_error()`, `WARN_PRINT()`, `ERR_PRINT()` for logging
- Use `ERR_FAIL_COND()`, `ERR_FAIL_INDEX()` for assertions with error messages
- Editor can be debugged by attaching debugger to running process

## Resource Loading

```cpp
Ref<Resource> res = ResourceLoader::load(path);
if (res.is_valid()) {
    // Use resource
}
```

## Scene Tree Interaction

```cpp
// Get nodes
Node *node = get_node(NodePath("path/to/node"));

// Iterate children
for (int i = 0; i < get_child_count(); i++) {
    Node *child = get_child(i);
}

// Scene management (editor context)
EditorNode::get_singleton()->get_edited_scene();  // Current scene root
EditorNode::get_singleton()->open_request("res://path.tscn");
```

## Web Platform Specifics

- Use `javascript_eval=yes` build flag to enable eval() for dynamic code execution
- Web builds output to `bin/` with `.wasm.br` Brotli compression
- Serve with proper headers: `Cross-Origin-Embedder-Policy: require-corp` and `Cross-Origin-Opener-Policy: same-origin`
- See `serve.js` for reference server implementation

## Custom Features in This Fork

This repository includes experimental "easy mode" feature:
- Hidden setting: `interface/editor/easy_mode` in EditorSettings
- Toggle via 10 consecutive clicks on version button (2s timeout)
- When active: hides title bar, sets 100% zoom default, hides canvas toolbar
- Version button shows "~" prefix when active
- Signal: `EditorNode::easy_mode_changed(bool enabled)`

### Easy Mode Implementation Pattern

**CRITICAL**: Easy mode must be **responsive** - components must subscribe to the `easy_mode_changed` signal rather than just reading the state once. This ensures the UI updates immediately when the user toggles easy mode.

```cpp
// WRONG - only reads state once at construction, won't respond to toggle
MyComponent::MyComponent() {
    if (EditorNode::get_singleton()->is_easy_mode()) {
        toolbar->set_visible(false);  // Won't update when easy mode changes!
    }
}

// CORRECT - subscribe to signal and apply initial state in NOTIFICATION_READY
void MyComponent::_on_easy_mode_changed(bool p_enabled) {
    toolbar->set_visible(!p_enabled);
}

void MyComponent::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        // Connect to signal for future changes
        EditorNode::get_singleton()->connect("easy_mode_changed", 
            callable_mp(this, &MyComponent::_on_easy_mode_changed));
        // Apply current state
        _on_easy_mode_changed(EditorNode::get_singleton()->is_easy_mode());
    }
}
```

Key implementation files:
- `editor/editor_node.h/cpp` - `easy_mode` member, `set_easy_mode()`, `is_easy_mode()`, emits `easy_mode_changed` signal
- `editor/editor_settings.cpp` - Hidden setting registration: `_initial_set("interface/editor/easy_mode", false, true)`
- `editor/gui/editor_version_button.cpp` - 10-click toggle mechanism with Timer
- `editor/plugins/canvas_item_editor_plugin.cpp` - Example of responsive easy mode (hides toolbar)
