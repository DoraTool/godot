/**************************************************************************/
/*  library_godot_editor_events.js                                      */
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

const GodotEditorEvents = {
	$GodotEditorEvents__deps: ['$GodotRuntime', '$GodotJSWrapper'],

	$GodotEditorEvents: {
		_saveCallback: null,

		/**
		 * Register a callback for editor save events.
		 * 
		 * @param {function} callback - Function to call when a save event occurs.
		 *                              Receives an event object: { type: 'scene' | 'resource', path: string, resourceType?: string }
		 */
		onSave: function (callback) {
			if (typeof callback !== 'function') {
				throw new Error('Callback must be a function');
			}

			// Store callback
			GodotEditorEvents.GodotEditorEvents._saveCallback = callback;

			// Get proxied ID for the callback function
			// GodotJSWrapper.get_proxied returns an ID that can be used to create JavaScriptObjectImpl
			const callbackId = GodotJSWrapper.get_proxied(callback);
			if (callbackId === undefined || callbackId === null) {
				throw new Error('Failed to create callback proxy');
			}

			// Call C binding to register the listener
			if (typeof Module !== 'undefined' && Module['_godot_js_add_save_listener']) {
				Module._godot_js_add_save_listener(callbackId);
			} else {
				throw new Error('Save listener API not available. Make sure the editor is running.');
			}
		},

		/**
		 * Unregister the save event listener.
		 */
		offSave: function () {
			GodotEditorEvents.GodotEditorEvents._saveCallback = null;

			// Call C binding to unregister the listener
			if (typeof Module !== 'undefined' && Module['_godot_js_remove_save_listener']) {
				Module._godot_js_remove_save_listener();
			}
		},
	},
};

autoAddDeps(GodotEditorEvents, '$GodotEditorEvents');
mergeInto(LibraryManager.library, GodotEditorEvents);

// Expose to global scope for JavaScript access
if (typeof window !== 'undefined') {
	window['GodotEditorEvents'] = GodotEditorEvents.GodotEditorEvents;
}

