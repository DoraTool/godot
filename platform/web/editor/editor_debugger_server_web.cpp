/**************************************************************************/
/*  editor_debugger_server_web.cpp                                       */
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

#include "editor_debugger_server_web.h"

#include "core/os/os.h"
#include "core/string/ustring.h"
#include "platform/web/debugger/remote_debugger_peer_web.h"

EditorDebuggerServer *EditorDebuggerServerWeb::create(const String &p_protocol) {
	ERR_FAIL_COND_V(p_protocol != "web://", nullptr);
	return memnew(EditorDebuggerServerWeb);
}

EditorDebuggerServerWeb::EditorDebuggerServerWeb() {}

String EditorDebuggerServerWeb::get_uri() const {
	return endpoint;
}

void EditorDebuggerServerWeb::poll() {
	// No background work required. Handshake happens through the JS bridge.
}

static String _web_debugger_generate_channel() {
	static uint64_t sequence = 0;
	OS *os = OS::get_singleton();
	uint64_t ticks = os ? os->get_ticks_usec() : 0;
	sequence++;
	return "session-" + String::num_uint64(ticks) + "-" + String::num_uint64(sequence);
}

Error EditorDebuggerServerWeb::start(const String &p_uri) {
	if (pending_peer.is_valid()) {
		pending_peer.unref();
	}

	String uri = p_uri;
	if (uri.is_empty() || uri == "web://") {
		uri = "web://" + _web_debugger_generate_channel();
	}

	ERR_FAIL_COND_V(!uri.begins_with("web://"), ERR_INVALID_PARAMETER);
	int sep = uri.find("://");
	String channel = uri.substr(sep + 3);
	if (channel.is_empty()) {
		channel = _web_debugger_generate_channel();
		uri = "web://" + channel;
	}

	pending_peer = Ref<RemoteDebuggerPeer>(memnew(RemoteDebuggerPeerWeb(channel, true)));
	endpoint = uri;
	active = true;
	return OK;
}

void EditorDebuggerServerWeb::stop() {
	pending_peer.unref();
	endpoint.clear();
	active = false;
}

bool EditorDebuggerServerWeb::is_active() const {
	return active;
}

bool EditorDebuggerServerWeb::is_connection_available() const {
	return pending_peer.is_valid() && pending_peer->is_peer_connected();
}

Ref<RemoteDebuggerPeer> EditorDebuggerServerWeb::take_connection() {
	ERR_FAIL_COND_V(!is_connection_available(), Ref<RemoteDebuggerPeer>());
	Ref<RemoteDebuggerPeer> peer = pending_peer;
	pending_peer.unref();
	return peer;
}
