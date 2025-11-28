/**************************************************************************/
/*  javascript_bridge_singleton.cpp                                       */
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

#include "api/javascript_bridge_singleton.h"

#include "os_web.h"

#include <emscripten.h>
#include <cstdlib>
#include <cstring>

#ifdef TOOLS_ENABLED
#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/object.h"
#include "editor/editor_node.h"
#include "editor/export/editor_export.h"
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
#include "core/io/json.h"
#include "scene/debugger/scene_debugger.h"
#endif // DEBUG_ENABLED

extern "C" {
extern void godot_js_os_download_buffer(const uint8_t *p_buf, int p_buf_size, const char *p_name, const char *p_mime);
}

#ifdef JAVASCRIPT_EVAL_ENABLED

extern "C" {
typedef union {
	int64_t i;
	double r;
	void *p;
} godot_js_wrapper_ex;

typedef int (*GodotJSWrapperVariant2JSCallback)(const void **p_args, int p_pos, godot_js_wrapper_ex *r_val, void **p_lock);
typedef void (*GodotJSWrapperFreeLockCallback)(void **p_lock, int p_type);
extern int godot_js_wrapper_interface_get(const char *p_name);
extern int godot_js_wrapper_object_call(int p_id, const char *p_method, void **p_args, int p_argc, GodotJSWrapperVariant2JSCallback p_variant2js_callback, godot_js_wrapper_ex *p_cb_rval, void **p_lock, GodotJSWrapperFreeLockCallback p_lock_callback);
extern int godot_js_wrapper_object_get(int p_id, godot_js_wrapper_ex *p_val, const char *p_prop);
extern int godot_js_wrapper_object_getvar(int p_id, int p_type, godot_js_wrapper_ex *p_val);
extern int godot_js_wrapper_object_setvar(int p_id, int p_key_type, godot_js_wrapper_ex *p_key_ex, int p_val_type, godot_js_wrapper_ex *p_val_ex);
extern void godot_js_wrapper_object_set(int p_id, const char *p_name, int p_type, godot_js_wrapper_ex *p_val);
extern void godot_js_wrapper_object_unref(int p_id);
extern int godot_js_wrapper_create_cb(void *p_ref, void (*p_callback)(void *p_ref, int p_arg_id, int p_argc));
extern void godot_js_wrapper_object_set_cb_ret(int p_type, godot_js_wrapper_ex *p_val);
extern int godot_js_wrapper_create_object(const char *p_method, void **p_args, int p_argc, GodotJSWrapperVariant2JSCallback p_variant2js_callback, godot_js_wrapper_ex *p_cb_rval, void **p_lock, GodotJSWrapperFreeLockCallback p_lock_callback);
extern int godot_js_wrapper_object_is_buffer(int p_id);
extern int godot_js_wrapper_object_transfer_buffer(int p_id, void *p_byte_arr, void *p_byte_arr_write, void *(*p_callback)(void *p_ptr, void *p_ptr2, int p_len));
};

class JavaScriptObjectImpl : public JavaScriptObject {
private:
	friend class JavaScriptBridge;

	int _js_id = 0;
	Callable _callable;

	WASM_EXPORT static int _variant2js(const void **p_args, int p_pos, godot_js_wrapper_ex *r_val, void **p_lock);
	WASM_EXPORT static void _free_lock(void **p_lock, int p_type);
	WASM_EXPORT static Variant _js2variant(int p_type, godot_js_wrapper_ex *p_val);
	WASM_EXPORT static void *_alloc_variants(int p_size);
	WASM_EXPORT static void callback(void *p_ref, int p_arg_id, int p_argc);
	static void _callback(const JavaScriptObjectImpl *obj, Variant arg);

protected:
	bool _set(const StringName &p_name, const Variant &p_value) override;
	bool _get(const StringName &p_name, Variant &r_ret) const override;
	void _get_property_list(List<PropertyInfo> *p_list) const override;

public:
	Variant getvar(const Variant &p_key, bool *r_valid = nullptr) const override;
	void setvar(const Variant &p_key, const Variant &p_value, bool *r_valid = nullptr) override;
	Variant callp(const StringName &p_method, const Variant **p_args, int p_argc, Callable::CallError &r_error) override;
	JavaScriptObjectImpl() {}
	JavaScriptObjectImpl(int p_id) { _js_id = p_id; }
	~JavaScriptObjectImpl() {
		if (_js_id) {
			godot_js_wrapper_object_unref(_js_id);
		}
	}
};

bool JavaScriptObjectImpl::_set(const StringName &p_name, const Variant &p_value) {
	ERR_FAIL_COND_V_MSG(!_js_id, false, "Invalid JS instance");
	const String name = p_name;
	godot_js_wrapper_ex exchange;
	void *lock = nullptr;
	const Variant *v = &p_value;
	int type = _variant2js((const void **)&v, 0, &exchange, &lock);
	godot_js_wrapper_object_set(_js_id, name.utf8().get_data(), type, &exchange);
	if (lock) {
		_free_lock(&lock, type);
	}
	return true;
}

bool JavaScriptObjectImpl::_get(const StringName &p_name, Variant &r_ret) const {
	ERR_FAIL_COND_V_MSG(!_js_id, false, "Invalid JS instance");
	const String name = p_name;
	godot_js_wrapper_ex exchange;
	int type = godot_js_wrapper_object_get(_js_id, &exchange, name.utf8().get_data());
	r_ret = _js2variant(type, &exchange);
	return true;
}

Variant JavaScriptObjectImpl::getvar(const Variant &p_key, bool *r_valid) const {
	if (r_valid) {
		*r_valid = false;
	}
	godot_js_wrapper_ex exchange;
	void *lock = nullptr;
	const Variant *v = &p_key;
	int prop_type = _variant2js((const void **)&v, 0, &exchange, &lock);
	int type = godot_js_wrapper_object_getvar(_js_id, prop_type, &exchange);
	if (lock) {
		_free_lock(&lock, prop_type);
	}
	if (type < 0) {
		return Variant();
	}
	if (r_valid) {
		*r_valid = true;
	}
	return _js2variant(type, &exchange);
}

void JavaScriptObjectImpl::setvar(const Variant &p_key, const Variant &p_value, bool *r_valid) {
	if (r_valid) {
		*r_valid = false;
	}
	godot_js_wrapper_ex kex, vex;
	void *klock = nullptr;
	void *vlock = nullptr;
	const Variant *kv = &p_key;
	const Variant *vv = &p_value;
	int ktype = _variant2js((const void **)&kv, 0, &kex, &klock);
	int vtype = _variant2js((const void **)&vv, 0, &vex, &vlock);
	int ret = godot_js_wrapper_object_setvar(_js_id, ktype, &kex, vtype, &vex);
	if (klock) {
		_free_lock(&klock, ktype);
	}
	if (vlock) {
		_free_lock(&vlock, vtype);
	}
	if (ret == 0 && r_valid) {
		*r_valid = true;
	}
}

void JavaScriptObjectImpl::_get_property_list(List<PropertyInfo> *p_list) const {
}

void JavaScriptObjectImpl::_free_lock(void **p_lock, int p_type) {
	ERR_FAIL_NULL_MSG(*p_lock, "No lock to free!");
	const Variant::Type type = (Variant::Type)p_type;
	switch (type) {
		case Variant::STRING: {
			CharString *cs = (CharString *)(*p_lock);
			memdelete(cs);
			*p_lock = nullptr;
		} break;
		default:
			ERR_FAIL_MSG("Unknown lock type to free. Likely a bug.");
	}
}

Variant JavaScriptObjectImpl::_js2variant(int p_type, godot_js_wrapper_ex *p_val) {
	Variant::Type type = (Variant::Type)p_type;
	switch (type) {
		case Variant::BOOL:
			return Variant((bool)p_val->i);
		case Variant::INT:
			return p_val->i;
		case Variant::FLOAT:
			return p_val->r;
		case Variant::STRING: {
			String out = String::utf8((const char *)p_val->p);
			free(p_val->p);
			return out;
		}
		case Variant::OBJECT: {
			return memnew(JavaScriptObjectImpl(p_val->i));
		}
		default:
			return Variant();
	}
}

int JavaScriptObjectImpl::_variant2js(const void **p_args, int p_pos, godot_js_wrapper_ex *r_val, void **p_lock) {
	const Variant **args = (const Variant **)p_args;
	const Variant *v = args[p_pos];
	Variant::Type type = v->get_type();
	switch (type) {
		case Variant::BOOL:
			r_val->i = v->operator bool() ? 1 : 0;
			break;
		case Variant::INT: {
			const int64_t tmp = v->operator int64_t();
			if (tmp >= 1LL << 31) {
				r_val->r = (double)tmp;
				return Variant::FLOAT;
			}
			r_val->i = v->operator int64_t();
		} break;
		case Variant::FLOAT:
			r_val->r = v->operator real_t();
			break;
		case Variant::STRING: {
			CharString *cs = memnew(CharString(v->operator String().utf8()));
			r_val->p = (void *)cs->get_data();
			*p_lock = (void *)cs;
		} break;
		case Variant::OBJECT: {
			JavaScriptObject *js_obj = Object::cast_to<JavaScriptObject>(v->operator Object *());
			r_val->i = js_obj != nullptr ? ((JavaScriptObjectImpl *)js_obj)->_js_id : 0;
		} break;
		default:
			break;
	}
	return type;
}

Variant JavaScriptObjectImpl::callp(const StringName &p_method, const Variant **p_args, int p_argc, Callable::CallError &r_error) {
	godot_js_wrapper_ex exchange;
	const String method = p_method;
	void *lock = nullptr;
	const int type = godot_js_wrapper_object_call(_js_id, method.utf8().get_data(), (void **)p_args, p_argc, &_variant2js, &exchange, &lock, &_free_lock);
	r_error.error = Callable::CallError::CALL_OK;
	if (type < 0) {
		r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Variant();
	}
	return _js2variant(type, &exchange);
}

void JavaScriptObjectImpl::callback(void *p_ref, int p_args_id, int p_argc) {
	const JavaScriptObjectImpl *obj = (JavaScriptObjectImpl *)p_ref;
	ERR_FAIL_COND_MSG(!obj->_callable.is_valid(), "JavaScript callback failed.");

	Vector<const Variant *> argp;
	Array arg_arr;
	for (int i = 0; i < p_argc; i++) {
		godot_js_wrapper_ex exchange;
		exchange.i = i;
		int type = godot_js_wrapper_object_getvar(p_args_id, Variant::INT, &exchange);
		arg_arr.push_back(_js2variant(type, &exchange));
	}
	Variant arg = arg_arr;

#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(JavaScriptObjectImpl::_callback).call_deferred(obj, arg);
		return;
	}
#endif

	_callback(obj, arg);
}

void JavaScriptObjectImpl::_callback(const JavaScriptObjectImpl *obj, Variant arg) {
	obj->_callable.call(arg);

	// Set return value
	godot_js_wrapper_ex exchange;
	void *lock = nullptr;
	Variant ret;
	const Variant *v = &ret;
	int type = _variant2js((const void **)&v, 0, &exchange, &lock);
	godot_js_wrapper_object_set_cb_ret(type, &exchange);
	if (lock) {
		_free_lock(&lock, type);
	}
}

Ref<JavaScriptObject> JavaScriptBridge::create_callback(const Callable &p_callable) {
	Ref<JavaScriptObjectImpl> out = memnew(JavaScriptObjectImpl);
	out->_callable = p_callable;
	out->_js_id = godot_js_wrapper_create_cb(out.ptr(), JavaScriptObjectImpl::callback);
	return out;
}

Ref<JavaScriptObject> JavaScriptBridge::get_interface(const String &p_interface) {
	int js_id = godot_js_wrapper_interface_get(p_interface.utf8().get_data());
	ERR_FAIL_COND_V_MSG(!js_id, Ref<JavaScriptObject>(), "No interface '" + p_interface + "' registered.");
	return Ref<JavaScriptObject>(memnew(JavaScriptObjectImpl(js_id)));
}

Variant JavaScriptBridge::_create_object_bind(const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	if (p_argcount < 1) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = 1;
		return Ref<JavaScriptObject>();
	}
	if (!p_args[0]->is_string()) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		r_error.argument = 0;
		r_error.expected = Variant::STRING;
		return Ref<JavaScriptObject>();
	}
	godot_js_wrapper_ex exchange;
	const String object = *p_args[0];
	void *lock = nullptr;
	const Variant **args = p_argcount > 1 ? &p_args[1] : nullptr;
	const int type = godot_js_wrapper_create_object(object.utf8().get_data(), (void **)args, p_argcount - 1, &JavaScriptObjectImpl::_variant2js, &exchange, &lock, &JavaScriptObjectImpl::_free_lock);
	r_error.error = Callable::CallError::CALL_OK;
	if (type < 0) {
		r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Ref<JavaScriptObject>();
	}
	return JavaScriptObjectImpl::_js2variant(type, &exchange);
}

extern "C" {
union js_eval_ret {
	uint32_t b;
	double d;
	char *s;
};

extern int godot_js_eval(const char *p_js, int p_use_global_ctx, union js_eval_ret *p_union_ptr, void *p_byte_arr, void *p_byte_arr_write, void *(*p_callback)(void *p_ptr, void *p_ptr2, int p_len));
}

void *resize_PackedByteArray_and_open_write(void *p_arr, void *r_write, int p_len) {
	PackedByteArray *arr = (PackedByteArray *)p_arr;
	VectorWriteProxy<uint8_t> *write = (VectorWriteProxy<uint8_t> *)r_write;
	arr->resize(p_len);
	*write = arr->write;
	return arr->ptrw();
}

Variant JavaScriptBridge::eval(const String &p_code, bool p_use_global_exec_context) {
	union js_eval_ret js_data;
	PackedByteArray arr;
	VectorWriteProxy<uint8_t> arr_write;

	Variant::Type return_type = static_cast<Variant::Type>(godot_js_eval(p_code.utf8().get_data(), p_use_global_exec_context, &js_data, &arr, &arr_write, resize_PackedByteArray_and_open_write));

	switch (return_type) {
		case Variant::BOOL:
			return js_data.b;
		case Variant::FLOAT:
			return js_data.d;
		case Variant::STRING: {
			String str = String::utf8(js_data.s);
			free(js_data.s); // Must free the string allocated in JS.
			return str;
		}
		case Variant::PACKED_BYTE_ARRAY:
			arr_write = VectorWriteProxy<uint8_t>();
			return arr;
		default:
			return Variant();
	}
}

bool JavaScriptBridge::is_js_buffer(Ref<JavaScriptObject> p_js_obj) {
	Ref<JavaScriptObjectImpl> obj = p_js_obj;
	if (obj.is_null()) {
		return false;
	}
	return godot_js_wrapper_object_is_buffer(obj->_js_id);
}

PackedByteArray JavaScriptBridge::js_buffer_to_packed_byte_array(Ref<JavaScriptObject> p_js_obj) {
	ERR_FAIL_COND_V_MSG(!is_js_buffer(p_js_obj), PackedByteArray(), "The JavaScript object is not a buffer.");
	Ref<JavaScriptObjectImpl> obj = p_js_obj;

	PackedByteArray arr;
	VectorWriteProxy<uint8_t> arr_write;

	godot_js_wrapper_object_transfer_buffer(obj->_js_id, &arr, &arr_write, resize_PackedByteArray_and_open_write);

	arr_write = VectorWriteProxy<uint8_t>();
	return arr;
}

#endif // JAVASCRIPT_EVAL_ENABLED

void JavaScriptBridge::download_buffer(Vector<uint8_t> p_arr, const String &p_name, const String &p_mime) {
	godot_js_os_download_buffer(p_arr.ptr(), p_arr.size(), p_name.utf8().get_data(), p_mime.utf8().get_data());
}

bool JavaScriptBridge::pwa_needs_update() const {
	return OS_Web::get_singleton()->pwa_needs_update();
}

Error JavaScriptBridge::pwa_update() {
	return OS_Web::get_singleton()->pwa_update();
}

void JavaScriptBridge::force_fs_sync() {
	OS_Web::get_singleton()->force_fs_sync();
}

#ifdef TOOLS_ENABLED

PackedByteArray JavaScriptBridge::export_pack(const String &p_preset_name, bool p_debug) {
	ERR_FAIL_COND_V_MSG(!Engine::get_singleton() || !Engine::get_singleton()->is_editor_hint(), PackedByteArray(), "Export is only available in editor mode.");

	EditorExport *ee = EditorExport::get_singleton();
	ERR_FAIL_COND_V_MSG(!ee, PackedByteArray(), "EditorExport singleton not available.");

	// Find the export preset
	Ref<EditorExportPreset> preset;
	if (p_preset_name.is_empty()) {
		// Use first available preset
		if (ee->get_export_preset_count() == 0) {
			ERR_FAIL_V_MSG(PackedByteArray(), "No export presets available.");
		}
		preset = ee->get_export_preset(0);
	} else {
		// Find preset by name
		bool found = false;
		for (int i = 0; i < ee->get_export_preset_count(); i++) {
			Ref<EditorExportPreset> p = ee->get_export_preset(i);
			if (p->get_name() == p_preset_name) {
				preset = p;
				found = true;
				break;
			}
		}
		ERR_FAIL_COND_V_MSG(!found, PackedByteArray(), vformat("Export preset '%s' not found.", p_preset_name));
	}

	ERR_FAIL_COND_V_MSG(preset.is_null(), PackedByteArray(), "Invalid export preset.");

	Ref<EditorExportPlatform> platform = preset->get_platform();
	ERR_FAIL_COND_V_MSG(platform.is_null(), PackedByteArray(), "Export platform not available.");

	// Create temporary file path
	const String temp_path = String("/tmp").path_join("export_temp.pck");

	// Export to temp file
	Error err = platform->export_pack(preset, p_debug, temp_path);
	if (err != OK) {
		ERR_FAIL_V_MSG(PackedByteArray(), vformat("Failed to export PCK: %s", error_names[err]));
	}

	// Read the file back
	Ref<FileAccess> f = FileAccess::open(temp_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), PackedByteArray(), "Failed to read exported PCK file.");

	PackedByteArray buffer;
	buffer.resize(f->get_length());
	f->get_buffer(buffer.ptrw(), buffer.size());
	f.unref();

	// Cleanup temp file
	DirAccess::remove_file_or_error(temp_path);

	return buffer;
}

PackedByteArray JavaScriptBridge::export_pack_patch(const String &p_preset_name, bool p_debug, const PackedStringArray &p_patches) {
	ERR_FAIL_COND_V_MSG(!Engine::get_singleton() || !Engine::get_singleton()->is_editor_hint(), PackedByteArray(), "Export is only available in editor mode.");

	EditorExport *ee = EditorExport::get_singleton();
	ERR_FAIL_COND_V_MSG(!ee, PackedByteArray(), "EditorExport singleton not available.");

	// Find the export preset
	Ref<EditorExportPreset> preset;
	if (p_preset_name.is_empty()) {
		// Use first available preset
		if (ee->get_export_preset_count() == 0) {
			ERR_FAIL_V_MSG(PackedByteArray(), "No export presets available.");
		}
		preset = ee->get_export_preset(0);
	} else {
		// Find preset by name
		bool found = false;
		for (int i = 0; i < ee->get_export_preset_count(); i++) {
			Ref<EditorExportPreset> p = ee->get_export_preset(i);
			if (p->get_name() == p_preset_name) {
				preset = p;
				found = true;
				break;
			}
		}
		ERR_FAIL_COND_V_MSG(!found, PackedByteArray(), vformat("Export preset '%s' not found.", p_preset_name));
	}

	ERR_FAIL_COND_V_MSG(preset.is_null(), PackedByteArray(), "Invalid export preset.");

	Ref<EditorExportPlatform> platform = preset->get_platform();
	ERR_FAIL_COND_V_MSG(platform.is_null(), PackedByteArray(), "Export platform not available.");

	// Convert PackedStringArray to Vector<String>
	Vector<String> patches_vec;
	for (int i = 0; i < p_patches.size(); i++) {
		patches_vec.push_back(p_patches[i]);
	}

	// Create temporary file path
	const String temp_path = String("/tmp").path_join("export_temp_patch.pck");

	// Export to temp file
	Error err = platform->export_pack_patch(preset, p_debug, temp_path, patches_vec);
	if (err != OK) {
		ERR_FAIL_V_MSG(PackedByteArray(), vformat("Failed to export PCK patch: %s", error_names[err]));
	}

	// Read the file back
	Ref<FileAccess> f = FileAccess::open(temp_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), PackedByteArray(), "Failed to read exported PCK patch file.");

	PackedByteArray buffer;
	buffer.resize(f->get_length());
	f->get_buffer(buffer.ptrw(), buffer.size());
	f.unref();

	// Cleanup temp file
	DirAccess::remove_file_or_error(temp_path);

	return buffer;
}

void JavaScriptBridge::add_save_listener(Ref<JavaScriptObject> p_callback) {
	ERR_FAIL_COND_MSG(!Engine::get_singleton() || !Engine::get_singleton()->is_editor_hint(), "Save listener is only available in editor mode.");

	if (save_listener_callback.is_valid()) {
		remove_save_listener();
	}

	save_listener_callback = p_callback;

	EditorNode *editor = EditorNode::get_singleton();
	if (editor) {
		editor->connect("scene_saved", callable_mp(this, &JavaScriptBridge::_on_scene_saved));
		editor->connect("resource_saved", callable_mp(this, &JavaScriptBridge::_on_resource_saved));
	}
}

void JavaScriptBridge::remove_save_listener() {
	if (!save_listener_callback.is_valid()) {
		return;
	}

	EditorNode *editor = EditorNode::get_singleton();
	if (editor) {
		editor->disconnect("scene_saved", callable_mp(this, &JavaScriptBridge::_on_scene_saved));
		editor->disconnect("resource_saved", callable_mp(this, &JavaScriptBridge::_on_resource_saved));
	}

	save_listener_callback.unref();
}

void JavaScriptBridge::_on_scene_saved(const String &p_path) {
	if (!save_listener_callback.is_valid()) {
		return;
	}

	Variant null_this;
	Variant type_var = String("scene");
	Variant path_var = p_path;
	const Variant *args[] = { &null_this, &type_var, &path_var };
	Callable::CallError error;
	save_listener_callback->callp("call", args, 3, error);
}

void JavaScriptBridge::_on_resource_saved(const Ref<Resource> &p_resource) {
	if (!save_listener_callback.is_valid() || !p_resource.is_valid()) {
		return;
	}

	Variant null_this;
	Variant type_var = String("resource");
	Variant path_var = p_resource->get_path();
	const Variant *args[] = { &null_this, &type_var, &path_var };
	Callable::CallError error;
	save_listener_callback->callp("call", args, 3, error);
}

extern "C" {
// Export functions for JavaScript
EMSCRIPTEN_KEEPALIVE
void *godot_js_export_pack(const char *p_preset_name, int p_debug, int *p_size) {
	JavaScriptBridge *bridge = JavaScriptBridge::get_singleton();
	if (!bridge) {
		*p_size = 0;
		return nullptr;
	}
	PackedByteArray buffer = bridge->export_pack(String::utf8(p_preset_name), p_debug != 0);
	if (buffer.is_empty()) {
		*p_size = 0;
		return nullptr;
	}
	*p_size = buffer.size();
	// Allocate memory and copy data (caller must free)
	void *result = malloc(buffer.size());
	memcpy(result, buffer.ptr(), buffer.size());
	return result;
}

EMSCRIPTEN_KEEPALIVE
void *godot_js_export_pack_patch(const char *p_preset_name, int p_debug, const char **p_patches, int p_patches_count, int *p_size) {
	JavaScriptBridge *bridge = JavaScriptBridge::get_singleton();
	if (!bridge) {
		*p_size = 0;
		return nullptr;
	}
	PackedStringArray patches;
	for (int i = 0; i < p_patches_count; i++) {
		patches.push_back(String::utf8(p_patches[i]));
	}
	PackedByteArray buffer = bridge->export_pack_patch(String::utf8(p_preset_name), p_debug != 0, patches);
	if (buffer.is_empty()) {
		*p_size = 0;
		return nullptr;
	}
	*p_size = buffer.size();
	// Allocate memory and copy data (caller must free)
	void *result = malloc(buffer.size());
	memcpy(result, buffer.ptr(), buffer.size());
	return result;
}
EMSCRIPTEN_KEEPALIVE
void godot_js_add_save_listener(int p_callback_id) {
#ifdef TOOLS_ENABLED
	JavaScriptBridge *bridge = JavaScriptBridge::get_singleton();
	if (!bridge) {
		return;
	}
	// Create JavaScriptObject from the callback ID
	Ref<JavaScriptObjectImpl> callback_obj = memnew(JavaScriptObjectImpl(p_callback_id));
	bridge->add_save_listener(callback_obj);
#endif
}

EMSCRIPTEN_KEEPALIVE
void godot_js_remove_save_listener() {
#ifdef TOOLS_ENABLED
	JavaScriptBridge *bridge = JavaScriptBridge::get_singleton();
	if (!bridge) {
		return;
	}
	bridge->remove_save_listener();
#endif
}

} // extern "C"

#endif // TOOLS_ENABLED

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"

extern "C" {
EMSCRIPTEN_KEEPALIVE
void godot_js_reload_current_scene() {
	print_line("[Hot Reload] Starting scene reload process...");

	SceneTree *tree = SceneTree::get_singleton();
	if (!tree) {
		ERR_PRINT("[Hot Reload] Cannot reload scene: SceneTree singleton not available.");
		return;
	}

	print_line("[Hot Reload] Reloading current scene tree...");
	Node *current_scene = tree->get_current_scene();
	if (!current_scene) {
		print_line("[Hot Reload] No current scene set, skipping scene tree reload.");
		return;
	}

	String current_scene_path = current_scene->get_scene_file_path();
	if (current_scene_path.is_empty()) {
		print_line("[Hot Reload] Current scene has no file path, skipping scene tree reload.");
		return;
	}

	Error err = tree->reload_current_scene();
	if (err != OK) {
		ERR_PRINT(vformat("[Hot Reload] Failed to reload current scene '%s': %s", current_scene_path, error_names[err]));
		return;
	}

	print_line("[Hot Reload] Scene reload process completed.");
}
} // extern "C"
