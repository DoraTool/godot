/**************************************************************************/
/*  library_godot_export.js                                              */
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

const GodotExport = {
	$GodotExport__deps: ['$GodotRuntime'],

	$GodotExport: {
		/**
		 * Export project to PCK file as ArrayBuffer.
		 * 
		 * @param {string} preset_name - Name of the export preset (empty string for first available)
		 * @param {boolean} debug - Whether to export debug version
		 * @returns {Promise<ArrayBuffer>} Promise that resolves to PCK file data as ArrayBuffer
		 */
		exportPack: function (preset_name, debug) {
			return new Promise(function (resolve, reject) {
				try {
					const preset_name_ptr = GodotRuntime.allocString(preset_name || '');
					const debug_int = debug ? 1 : 0;
					const size_ptr = Module._malloc(4); // int size
					
					const buffer_ptr = Module._godot_js_export_pack(preset_name_ptr, debug_int, size_ptr);
					
					// Free string
					Module._free(preset_name_ptr);
					
					if (!buffer_ptr) {
						Module._free(size_ptr);
						reject(new Error('Failed to export PCK'));
						return;
					}
					
					const size = Module.HEAP32[size_ptr >> 2];
					Module._free(size_ptr);
					
					if (size === 0) {
						Module._free(buffer_ptr);
						reject(new Error('Export returned empty buffer'));
						return;
					}
					
					// Copy data to ArrayBuffer
					const buffer = new ArrayBuffer(size);
					const view = new Uint8Array(buffer);
					view.set(Module.HEAPU8.subarray(buffer_ptr, buffer_ptr + size));
					
					// Free allocated memory
					Module._free(buffer_ptr);
					
					resolve(buffer);
				} catch (e) {
					reject(e);
				}
			});
		},

		/**
		 * Export project to PCK patch file as ArrayBuffer.
		 * 
		 * @param {string} preset_name - Name of the export preset (empty string for first available)
		 * @param {boolean} debug - Whether to export debug version
		 * @param {string[]} patches - Array of patch file paths
		 * @returns {Promise<ArrayBuffer>} Promise that resolves to PCK patch file data as ArrayBuffer
		 */
		exportPackPatch: function (preset_name, debug, patches) {
			return new Promise(function (resolve, reject) {
				try {
					const preset_name_ptr = GodotRuntime.allocString(preset_name || '');
					const debug_int = debug ? 1 : 0;
					const patches_count = patches ? patches.length : 0;
					
					// Allocate array of string pointers
					const patches_ptr = Module._malloc(patches_count * 4); // pointers are 4 bytes in wasm32
					const patches_ptrs = [];
					
					for (let i = 0; i < patches_count; i++) {
						const str_ptr = GodotRuntime.allocString(patches[i]);
						patches_ptrs.push(str_ptr);
						Module.HEAP32[(patches_ptr >> 2) + i] = str_ptr;
					}
					
					const size_ptr = Module._malloc(4); // int size
					
					const buffer_ptr = Module._godot_js_export_pack_patch(
						preset_name_ptr,
						debug_int,
						patches_ptr,
						patches_count,
						size_ptr
					);
					
					// Free strings
					Module._free(preset_name_ptr);
					for (let i = 0; i < patches_ptrs.length; i++) {
						Module._free(patches_ptrs[i]);
					}
					Module._free(patches_ptr);
					
					if (!buffer_ptr) {
						Module._free(size_ptr);
						reject(new Error('Failed to export PCK patch'));
						return;
					}
					
					const size = Module.HEAP32[size_ptr >> 2];
					Module._free(size_ptr);
					
					if (size === 0) {
						Module._free(buffer_ptr);
						reject(new Error('Export returned empty buffer'));
						return;
					}
					
					// Copy data to ArrayBuffer
					const buffer = new ArrayBuffer(size);
					const view = new Uint8Array(buffer);
					view.set(Module.HEAPU8.subarray(buffer_ptr, buffer_ptr + size));
					
					// Free allocated memory
					Module._free(buffer_ptr);
					
					resolve(buffer);
				} catch (e) {
					reject(e);
				}
			});
		},
	},

};

autoAddDeps(GodotExport, '$GodotExport');
mergeInto(LibraryManager.library, GodotExport);

// Expose to global scope for JavaScript access
if (typeof window !== 'undefined') {
	window['GodotExport'] = GodotExport.GodotExport;
}

