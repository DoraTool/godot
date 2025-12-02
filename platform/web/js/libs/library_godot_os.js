/**************************************************************************/
/*  library_godot_os.js                                                   */
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

const IDHandler = {
	$IDHandler: {
		_last_id: 0,
		_references: {},

		get: function (p_id) {
			return IDHandler._references[p_id];
		},

		add: function (p_data) {
			const id = ++IDHandler._last_id;
			IDHandler._references[id] = p_data;
			return id;
		},

		remove: function (p_id) {
			delete IDHandler._references[p_id];
		},
	},
};

autoAddDeps(IDHandler, '$IDHandler');
mergeInto(LibraryManager.library, IDHandler);

const GodotConfig = {
	$GodotConfig__postset: 'Module["initConfig"] = GodotConfig.init_config;',
	$GodotConfig__deps: ['$GodotRuntime'],
	$GodotConfig: {
		canvas: null,
		locale: 'en',
		canvas_resize_policy: 2, // Adaptive
		virtual_keyboard: false,
		persistent_drops: false,
		on_execute: null,
		on_exit: null,

		init_config: function (p_opts) {
			GodotConfig.canvas_resize_policy = p_opts['canvasResizePolicy'];
			GodotConfig.canvas = p_opts['canvas'];
			GodotConfig.locale = p_opts['locale'] || GodotConfig.locale;
			GodotConfig.virtual_keyboard = p_opts['virtualKeyboard'];
			GodotConfig.persistent_drops = !!p_opts['persistentDrops'];
			GodotConfig.on_execute = p_opts['onExecute'];
			GodotConfig.on_exit = p_opts['onExit'];
			if (p_opts['focusCanvas']) {
				GodotConfig.canvas.focus();
			}
		},

		locate_file: function (file) {
			return Module['locateFile'](file);
		},
		clear: function () {
			GodotConfig.canvas = null;
			GodotConfig.locale = 'en';
			GodotConfig.canvas_resize_policy = 2;
			GodotConfig.virtual_keyboard = false;
			GodotConfig.persistent_drops = false;
			GodotConfig.on_execute = null;
			GodotConfig.on_exit = null;
		},
	},

	godot_js_config_canvas_id_get__proxy: 'sync',
	godot_js_config_canvas_id_get__sig: 'vii',
	godot_js_config_canvas_id_get: function (p_ptr, p_ptr_max) {
		GodotRuntime.stringToHeap(`#${GodotConfig.canvas.id}`, p_ptr, p_ptr_max);
	},

	godot_js_config_locale_get__proxy: 'sync',
	godot_js_config_locale_get__sig: 'vii',
	godot_js_config_locale_get: function (p_ptr, p_ptr_max) {
		GodotRuntime.stringToHeap(GodotConfig.locale, p_ptr, p_ptr_max);
	},
};

autoAddDeps(GodotConfig, '$GodotConfig');
mergeInto(LibraryManager.library, GodotConfig);

const GodotFS = {
	$GodotFS__deps: ['$FS', '$IDBFS', '$GodotRuntime'],
	$GodotFS__postset: [
		'Module["initFS"] = GodotFS.init;',
		'Module["copyToFS"] = GodotFS.copy_to_fs;',
		'Module["readFromFS"] = GodotFS.read_from_fs;',
		'Module["syncToDB"] = GodotFS.sync_to_db;',
		'Module["syncFromDB"] = GodotFS.sync_from_db;',
	].join(''),
	$GodotFS: {
		// ERRNO_CODES works every odd version of emscripten, but this will break too eventually.
		ENOENT: 44,
		_idbfs: false,
		_sync_promise: null,
		_mount_points: [],

		is_persistent: function () {
			return GodotFS._idbfs ? 1 : 0;
		},

		// Initialize godot file system, setting up persistent paths.
		// Returns a promise that resolves when the FS is ready.
		// We keep track of mount_points, so that we can properly close the IDBFS
		// since emscripten is not doing it by itself. (emscripten GH#12516).
		init: function (persistentPaths, fsSystem) {
			GodotFS._idbfs = false;
			if (!Array.isArray(persistentPaths)) {
				return Promise.reject(new Error('Persistent paths must be an array'));
			}
			if (!persistentPaths.length) {
				return Promise.resolve();
			}
			GodotFS._mount_points = persistentPaths.slice();

			function createRecursive(dir) {
				try {
					FS.stat(dir);
				} catch (e) {
					if (e.errno !== GodotFS.ENOENT) {
						// Let mkdirTree throw in case, we cannot trust the above check.
						GodotRuntime.error(e);
					}
					FS.mkdirTree(dir);
				}
			}

			GodotFS._mount_points.forEach(function (path) {
				createRecursive(path);
				FS.mount(fsSystem || IDBFS, {}, path);
			});
			return Promise.resolve();
			
			// return new Promise(function (resolve, reject) {
			// 	FS.syncfs(true, function (err) {
			// 		if (err) {
			// 			GodotFS._mount_points = [];
			// 			GodotFS._idbfs = false;
			// 			GodotRuntime.print(`IndexedDB not available: ${err.message}`);
			// 		} else {
			// 			GodotFS._idbfs = true;
			// 		}
			// 		resolve(err);
			// 	});
			// });
		},

		// Deinit godot file system, making sure to unmount file systems, and close IDBFS(s).
		deinit: function () {
			GodotFS._mount_points.forEach(function (path) {
				try {
					FS.unmount(path);
				} catch (e) {
					GodotRuntime.print('Already unmounted', e);
				}
				if (GodotFS._idbfs && IDBFS.dbs[path]) {
					IDBFS.dbs[path].close();
					delete IDBFS.dbs[path];
				}
			});
			GodotFS._mount_points = [];
			GodotFS._idbfs = false;
			GodotFS._sync_promise = null;
		},

		sync: function () {
			if (GodotFS._sync_promise) {
				return GodotFS._sync_promise;
			}
			const promise = new Promise(function (resolve, reject) {
				FS.syncfs(false, function (error) {
					if (error) {
						GodotRuntime.error(`Failed to save IDB file system: ${error.message}`);
					}
					GodotFS._sync_promise = null;
					resolve(error);
				});
			});
			GodotFS._sync_promise = promise;
			return promise;
		},

		// Sync file system from memory to IndexedDB (write sync).
		sync_to_db: function () {
			if (GodotFS._sync_promise) {
				return GodotFS._sync_promise;
			}
			const promise = new Promise(function (resolve, reject) {
				FS.syncfs(false, function (error) {
					if (error) {
						GodotRuntime.error(`Failed to sync file system to IndexedDB: ${error.message}`);
					}
					GodotFS._sync_promise = null;
					resolve(error);
				});
			});
			GodotFS._sync_promise = promise;
			return promise;
		},

		// Sync file system from IndexedDB to memory (read sync).
		sync_from_db: function () {
			if (GodotFS._sync_promise) {
				return GodotFS._sync_promise;
			}
			const promise = new Promise(function (resolve, reject) {
				FS.syncfs(true, function (error) {
					if (error) {
						GodotRuntime.error(`Failed to sync file system from IndexedDB: ${error.message}`);
					}
					GodotFS._sync_promise = null;
					resolve(error);
				});
			});
			GodotFS._sync_promise = promise;
			return promise;
		},

		// Copies a buffer to the internal file system. Creating directories recursively.
		copy_to_fs: function (path, buffer) {
			const idx = path.lastIndexOf('/');
			let dir = '/';
			if (idx > 0) {
				dir = path.slice(0, idx);
			}
			try {
				FS.stat(dir);
			} catch (e) {
				if (e.errno !== GodotFS.ENOENT) {
					// Let mkdirTree throw in case, we cannot trust the above check.
					GodotRuntime.error(e);
				}
				FS.mkdirTree(dir);
			}
			FS.writeFile(path, new Uint8Array(buffer));
		},

		// Reads a file from the internal file system.
		read_from_fs: function (path) {
			try {
				return FS.readFile(path);
			} catch (e) {
				if (e.errno === GodotFS.ENOENT) {
					// File doesn't exist, return null
					return null;
				}
				// Re-throw other errors
				throw e;
			}
		},
	},
};
mergeInto(LibraryManager.library, GodotFS);

const GodotOS = {
	$GodotOS__deps: ['$GodotRuntime', '$GodotConfig', '$GodotFS'],
	$GodotOS__postset: [
		'Module["request_quit"] = function() { GodotOS.request_quit() };',
		'Module["onExit"] = GodotOS.cleanup;',
		'GodotOS._fs_sync_promise = Promise.resolve();',
	].join(''),
	$GodotOS: {
		request_quit: function () { },
		_async_cbs: [],
		_fs_sync_promise: null,

		atexit: function (p_promise_cb) {
			GodotOS._async_cbs.push(p_promise_cb);
		},

		cleanup: function (exit_code) {
			const cb = GodotConfig.on_exit;
			GodotFS.deinit();
			GodotConfig.clear();
			if (cb) {
				cb(exit_code);
			}
		},

		finish_async: function (callback) {
			GodotOS._fs_sync_promise.then(function (err) {
				const promises = [];
				GodotOS._async_cbs.forEach(function (cb) {
					promises.push(new Promise(cb));
				});
				return Promise.all(promises);
			}).then(function () {
				return GodotFS.sync(); // Final FS sync.
			}).then(function (err) {
				// Always deferred.
				setTimeout(function () {
					callback();
				}, 0);
			});
		},
	},

	godot_js_os_finish_async__proxy: 'sync',
	godot_js_os_finish_async__sig: 'vi',
	godot_js_os_finish_async: function (p_callback) {
		const func = GodotRuntime.get_func(p_callback);
		GodotOS.finish_async(func);
	},

	godot_js_os_request_quit_cb__proxy: 'sync',
	godot_js_os_request_quit_cb__sig: 'vi',
	godot_js_os_request_quit_cb: function (p_callback) {
		GodotOS.request_quit = GodotRuntime.get_func(p_callback);
	},

	godot_js_os_fs_is_persistent__proxy: 'sync',
	godot_js_os_fs_is_persistent__sig: 'i',
	godot_js_os_fs_is_persistent: function () {
		return GodotFS.is_persistent();
	},

	godot_js_os_fs_sync__proxy: 'sync',
	godot_js_os_fs_sync__sig: 'vi',
	godot_js_os_fs_sync: function (callback) {
		const func = GodotRuntime.get_func(callback);
		GodotOS._fs_sync_promise = GodotFS.sync();
		GodotOS._fs_sync_promise.then(function (err) {
			func();
		});
	},

	godot_js_os_has_feature__proxy: 'sync',
	godot_js_os_has_feature__sig: 'ii',
	godot_js_os_has_feature: function (p_ftr) {
		const ftr = GodotRuntime.parseString(p_ftr);
		const ua = navigator.userAgent;
		if (ftr === 'web_macos') {
			return (ua.indexOf('Mac') !== -1) ? 1 : 0;
		}
		if (ftr === 'web_windows') {
			return (ua.indexOf('Windows') !== -1) ? 1 : 0;
		}
		if (ftr === 'web_android') {
			return (ua.indexOf('Android') !== -1) ? 1 : 0;
		}
		if (ftr === 'web_ios') {
			return ((ua.indexOf('iPhone') !== -1) || (ua.indexOf('iPad') !== -1) || (ua.indexOf('iPod') !== -1)) ? 1 : 0;
		}
		if (ftr === 'web_linuxbsd') {
			return ((ua.indexOf('CrOS') !== -1) || (ua.indexOf('BSD') !== -1) || (ua.indexOf('Linux') !== -1) || (ua.indexOf('X11') !== -1)) ? 1 : 0;
		}
		return 0;
	},

	godot_js_os_execute__proxy: 'sync',
	godot_js_os_execute__sig: 'ii',
	godot_js_os_execute: function (p_json) {
		const json_args = GodotRuntime.parseString(p_json);
		const args = JSON.parse(json_args);
		if (GodotConfig.on_execute) {
			GodotConfig.on_execute(args);
			return 0;
		}
		return 1;
	},

	godot_js_os_shell_open__proxy: 'sync',
	godot_js_os_shell_open__sig: 'vi',
	godot_js_os_shell_open: function (p_uri) {
		window.open(GodotRuntime.parseString(p_uri), '_blank');
	},

	godot_js_os_hw_concurrency_get__proxy: 'sync',
	godot_js_os_hw_concurrency_get__sig: 'i',
	godot_js_os_hw_concurrency_get: function () {
		// TODO Godot core needs fixing to avoid spawning too many threads (> 24).
		const concurrency = navigator.hardwareConcurrency || 1;
		return concurrency < 2 ? concurrency : 2;
	},

	godot_js_os_download_buffer__proxy: 'sync',
	godot_js_os_download_buffer__sig: 'viiii',
	godot_js_os_download_buffer: function (p_ptr, p_size, p_name, p_mime) {
		const buf = GodotRuntime.heapSlice(HEAP8, p_ptr, p_size);
		const name = GodotRuntime.parseString(p_name);
		const mime = GodotRuntime.parseString(p_mime);
		const blob = new Blob([buf], { type: mime });
		const url = window.URL.createObjectURL(blob);
		const a = document.createElement('a');
		a.href = url;
		a.download = name;
		a.style.display = 'none';
		document.body.appendChild(a);
		a.click();
		a.remove();
		window.URL.revokeObjectURL(url);
	},
};

autoAddDeps(GodotOS, '$GodotOS');
mergeInto(LibraryManager.library, GodotOS);

const GodotLiveDebug = {
	$GodotLiveDebug__deps: ['$GodotRuntime'],
	$GodotLiveDebug__postset: [
		'Module["startDebugServer"] = GodotLiveDebug.start_debug_server;',
		'Module["stopDebugServer"] = GodotLiveDebug.stop_debug_server;',
	].join(''),
	$GodotLiveDebug: {
		// Start the editor's debug server from JavaScript.
		// Usage: Module.startDebugServer("channel_name") or Module.startDebugServer() for default channel.
		// Returns 0 on success, non-zero on error.
		start_debug_server: function (channel) {
			const channelName = channel || 'default';
			if (!Module._godot_js_editor_start_debug_server) {
				GodotRuntime.error('startDebugServer is only available in the editor build');
				return -1;
			}
			const ptr = GodotRuntime.allocString(channelName);
			const result = Module._godot_js_editor_start_debug_server(ptr);
			GodotRuntime.free(ptr);
			return result;
		},

		// Stop the editor's debug server.
		stop_debug_server: function () {
			if (!Module._godot_js_editor_stop_debug_server) {
				GodotRuntime.error('stopDebugServer is only available in the editor build');
				return;
			}
			Module._godot_js_editor_stop_debug_server();
		},

		// Called during registration (in the module's own JS context) to capture
		// memory functions that are only available as globals in each module scope.
		_capture_module_helpers: function (module) {
			// Capture the global heap and memory functions from this module's scope.
			// These closures will retain references to the correct HEAPU8/_malloc/_free
			// for this specific WASM instance.
			module.__godotLiveDebugRead = function (ptr, len) {
				return HEAPU8.slice(ptr, ptr + len);
			};
			module.__godotLiveDebugWrite = function (ptr, data) {
				HEAPU8.set(data, ptr);
			};
			module.__godotLiveDebugMalloc = function (size) {
				return _malloc(size);
			};
			module.__godotLiveDebugFree = function (ptr) {
				_free(ptr);
			};
		},
		_get_bus: function () {
			if (!globalThis.__GodotLiveDebugBus) {
				globalThis.__GodotLiveDebugBus = {
					channels: Object.create(null),
				};
			}
			return globalThis.__GodotLiveDebugBus;
		},

		_get_channel: function (name) {
			const channelName = name || 'default';
			const bus = GodotLiveDebug._get_bus();
			if (!bus.channels[channelName]) {
				bus.channels[channelName] = {
					name: channelName,
					endpoints: [],
				};
			}
			return bus.channels[channelName];
		},

		_store_endpoint: function (module, endpoint) {
			if (!module.__godotLiveDebugEndpoints) {
				module.__godotLiveDebugEndpoints = Object.create(null);
			}
			module.__godotLiveDebugEndpoints[endpoint.handle] = endpoint;
		},

		_get_endpoint: function (module, handle) {
			if (!module.__godotLiveDebugEndpoints) {
				return null;
			}
			return module.__godotLiveDebugEndpoints[handle] || null;
		},

		_remove_endpoint: function (module, handle) {
			if (!module.__godotLiveDebugEndpoints) {
				return null;
			}
			const endpoint = module.__godotLiveDebugEndpoints[handle];
			delete module.__godotLiveDebugEndpoints[handle];
			return endpoint || null;
		},

		_emit_event: function (endpoint, code) {
			const module = endpoint.module;
			if (module && module._godot_js_live_debug_on_event) {
				module._godot_js_live_debug_on_event(endpoint.handle, code);
			}
		},

		_disconnect_endpoint: function (endpoint) {
			if (!endpoint) {
				return;
			}
			if (endpoint.partner) {
				const partner = endpoint.partner;
				endpoint.partner = null;
				endpoint.connected = false;
				partner.partner = null;
				partner.connected = false;
				GodotLiveDebug._emit_event(partner, 2);
			}
		},

		_try_connect: function (channel) {
			if (!channel || channel.endpoints.length < 2) {
				return;
			}
			let server = null;
			let client = null;
			for (let i = 0; i < channel.endpoints.length; i++) {
				const endpoint = channel.endpoints[i];
				if (endpoint.partner) {
					continue;
				}
				if (endpoint.isServer && !server) {
					server = endpoint;
				} else if (!endpoint.isServer && !client) {
					client = endpoint;
				}
				if (server && client) {
					break;
				}
			}
			if (server && client) {
				server.partner = client;
				client.partner = server;
				server.connected = true;
				client.connected = true;
				GodotLiveDebug._emit_event(server, 1);
				GodotLiveDebug._emit_event(client, 1);
			}
		},

		_register_peer: function (module, channelName, isServer) {
			// Capture module helpers at registration time - this is called from
			// within the module's own JS context, so globals like HEAPU8/_malloc
			// will be captured correctly for this specific WASM instance.
			GodotLiveDebug._capture_module_helpers(module);
			const channel = GodotLiveDebug._get_channel(channelName);
			module.__godotLiveDebugHandleSeed = (module.__godotLiveDebugHandleSeed || 0) + 1;
			const endpoint = {
				handle: module.__godotLiveDebugHandleSeed,
				module: module,
				channel: channel.name,
				isServer: !!isServer,
				partner: null,
				connected: false,
			};
			channel.endpoints.push(endpoint);
			GodotLiveDebug._store_endpoint(module, endpoint);
			GodotLiveDebug._try_connect(channel);
			return endpoint.handle;
		},

		_unregister_peer: function (module, handle) {
			const endpoint = GodotLiveDebug._remove_endpoint(module, handle);
			if (!endpoint) {
				return;
			}
			const bus = GodotLiveDebug._get_bus();
			const channel = bus.channels[endpoint.channel];
			if (channel) {
				const idx = channel.endpoints.indexOf(endpoint);
				if (idx >= 0) {
					channel.endpoints.splice(idx, 1);
				}
				GodotLiveDebug._disconnect_endpoint(endpoint);
				GodotLiveDebug._try_connect(channel);
				if (channel.endpoints.length === 0) {
					delete bus.channels[endpoint.channel];
				}
			}
		},

		_send: function (module, handle, dataPtr, dataLen) {
			const endpoint = GodotLiveDebug._get_endpoint(module, handle);
			if (!endpoint || !endpoint.connected || !endpoint.partner) {
				return 1;
			}
			// Copy the payload from the sender's heap using captured helper
			const payload = module.__godotLiveDebugRead(dataPtr, dataLen);
			const receiver = endpoint.partner;
			const receiverModule = receiver.module;
			// Check receiver has the callback and helpers
			if (!receiverModule || !receiverModule._godot_js_live_debug_on_payload) {
				return 2;
			}
			if (!receiverModule.__godotLiveDebugMalloc) {
				return 3;
			}
			// Allocate in receiver's heap using captured malloc
			const ptr = receiverModule.__godotLiveDebugMalloc(payload.length);
			if (!ptr) {
				return 4;
			}
			// Write to receiver's heap using captured helper
			receiverModule.__godotLiveDebugWrite(ptr, payload);
			// Call receiver's callback
			receiverModule._godot_js_live_debug_on_payload(receiver.handle, ptr, payload.length);
			receiverModule.__godotLiveDebugFree(ptr);
			return 0;
		},
	},

	godot_js_live_debug_register_peer__deps: ['$GodotLiveDebug'],
	godot_js_live_debug_register_peer__proxy: 'sync',
	godot_js_live_debug_register_peer__sig: 'iii',
	godot_js_live_debug_register_peer: function (p_channel, p_is_server) {
		const channel = GodotRuntime.parseString(p_channel);
		return GodotLiveDebug._register_peer(Module, channel, p_is_server);
	},

	godot_js_live_debug_unregister_peer__deps: ['$GodotLiveDebug'],
	godot_js_live_debug_unregister_peer__proxy: 'sync',
	godot_js_live_debug_unregister_peer__sig: 'vi',
	godot_js_live_debug_unregister_peer: function (p_handle) {
		GodotLiveDebug._unregister_peer(Module, p_handle);
	},

	godot_js_live_debug_send__deps: ['$GodotLiveDebug'],
	godot_js_live_debug_send__proxy: 'sync',
	godot_js_live_debug_send__sig: 'iiii',
	godot_js_live_debug_send: function (p_handle, p_data, p_length) {
		return GodotLiveDebug._send(Module, p_handle, p_data, p_length);
	},
};
mergeInto(LibraryManager.library, GodotLiveDebug);

/*
 * Godot event listeners.
 * Keeps track of registered event listeners so it can remove them on shutdown.
 */
const GodotEventListeners = {
	$GodotEventListeners__deps: ['$GodotOS'],
	$GodotEventListeners__postset: 'GodotOS.atexit(function(resolve, reject) { GodotEventListeners.clear(); resolve(); });',
	$GodotEventListeners: {
		handlers: [],

		has: function (target, event, method, capture) {
			return GodotEventListeners.handlers.findIndex(function (e) {
				return e.target === target && e.event === event && e.method === method && e.capture === capture;
			}) !== -1;
		},

		add: function (target, event, method, capture) {
			if (GodotEventListeners.has(target, event, method, capture)) {
				return;
			}
			function Handler(p_target, p_event, p_method, p_capture) {
				this.target = p_target;
				this.event = p_event;
				this.method = p_method;
				this.capture = p_capture;
			}
			GodotEventListeners.handlers.push(new Handler(target, event, method, capture));
			target.addEventListener(event, method, capture);
		},

		clear: function () {
			GodotEventListeners.handlers.forEach(function (h) {
				h.target.removeEventListener(h.event, h.method, h.capture);
			});
			GodotEventListeners.handlers.length = 0;
		},
	},
};
mergeInto(LibraryManager.library, GodotEventListeners);

const GodotPWA = {

	$GodotPWA__deps: ['$GodotRuntime', '$GodotEventListeners'],
	$GodotPWA: {
		hasUpdate: false,

		updateState: function (cb, reg) {
			if (!reg) {
				return;
			}
			if (!reg.active) {
				return;
			}
			if (reg.waiting) {
				GodotPWA.hasUpdate = true;
				cb();
			}
			GodotEventListeners.add(reg, 'updatefound', function () {
				const installing = reg.installing;
				GodotEventListeners.add(installing, 'statechange', function () {
					if (installing.state === 'installed') {
						GodotPWA.hasUpdate = true;
						cb();
					}
				});
			});
		},
	},

	godot_js_pwa_cb__proxy: 'sync',
	godot_js_pwa_cb__sig: 'vi',
	godot_js_pwa_cb: function (p_update_cb) {
		if ('serviceWorker' in navigator) {
			try {
				const cb = GodotRuntime.get_func(p_update_cb);
				navigator.serviceWorker.getRegistration().then(GodotPWA.updateState.bind(null, cb));
			} catch (e) {
				GodotRuntime.error('Failed to assign PWA callback', e);
			}
		}
	},

	godot_js_pwa_update__proxy: 'sync',
	godot_js_pwa_update__sig: 'i',
	godot_js_pwa_update: function () {
		if ('serviceWorker' in navigator && GodotPWA.hasUpdate) {
			try {
				navigator.serviceWorker.getRegistration().then(function (reg) {
					if (!reg || !reg.waiting) {
						return;
					}
					reg.waiting.postMessage('update');
				});
			} catch (e) {
				GodotRuntime.error(e);
				return 1;
			}
			return 0;
		}
		return 1;
	},
};

autoAddDeps(GodotPWA, '$GodotPWA');
mergeInto(LibraryManager.library, GodotPWA);
