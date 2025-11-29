/**************************************************************************/
/*  remote_debugger_peer_web.cpp                                         */
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

#include "remote_debugger_peer_web.h"

#include "core/io/marshalls.h"
#include "core/os/os.h"

#include <emscripten.h>

Mutex RemoteDebuggerPeerWeb::peer_map_mutex;
HashMap<int32_t, RemoteDebuggerPeerWeb *> RemoteDebuggerPeerWeb::peer_map;

extern "C" {
int godot_js_live_debug_register_peer(const char *p_channel, int p_is_server);
void godot_js_live_debug_unregister_peer(int p_handle);
int godot_js_live_debug_send(int p_handle, const uint8_t *p_data, int p_length);
}

RemoteDebuggerPeerWeb::RemoteDebuggerPeerWeb(const String &p_channel, bool p_is_server) {
	channel = p_channel;
	server_endpoint = p_is_server;
	CharString channel_utf8 = channel.utf8();
	handle = godot_js_live_debug_register_peer(channel_utf8.get_data(), server_endpoint ? 1 : 0);
	ERR_FAIL_COND_MSG(handle <= 0, "Failed to create live-debug channel for web transport.");

	MutexLock lock(peer_map_mutex);
	peer_map.insert(handle, this);
}

RemoteDebuggerPeerWeb::~RemoteDebuggerPeerWeb() {
	close();
}

void RemoteDebuggerPeerWeb::_set_connected(bool p_connected) {
	MutexLock lock(mutex);
	connected = p_connected;
	if (!connected) {
		in_queue.clear();
	}
}

void RemoteDebuggerPeerWeb::_push_packet(const uint8_t *p_data, int p_length) {
	Variant payload;
	int read = 0;
	Error err = decode_variant(payload, p_data, p_length, &read);
	ERR_FAIL_COND_MSG(err != OK, "Failed to decode remote debugger payload.");
	ERR_FAIL_COND_MSG(payload.get_type() != Variant::ARRAY, "Remote debugger payload is not an Array.");

	MutexLock lock(mutex);
	if (in_queue.size() >= max_queued_messages) {
		in_queue.pop_front();
	}
	in_queue.push_back(payload);
}

RemoteDebuggerPeer *RemoteDebuggerPeerWeb::create(const String &p_uri) {
	ERR_FAIL_COND_V(!p_uri.begins_with("web://"), nullptr);
	const int sep = p_uri.find("://");
	String channel_name = p_uri.substr(sep + 3);
	ERR_FAIL_COND_V(channel_name.is_empty(), nullptr);
	return memnew(RemoteDebuggerPeerWeb(channel_name, false));
}

void RemoteDebuggerPeerWeb::handle_event(int p_handle, int p_event) {
	RemoteDebuggerPeerWeb *peer = nullptr;
	{
		MutexLock lock(peer_map_mutex);
		RemoteDebuggerPeerWeb **ptr = peer_map.getptr(p_handle);
		if (ptr) {
			peer = *ptr;
		}
	}

	if (!peer) {
		return;
	}

	switch (p_event) {
		case EVENT_CONNECTED: {
			peer->_set_connected(true);
		} break;
		case EVENT_DISCONNECTED: {
			peer->_set_connected(false);
		} break;
		default:
			break;
	}
}

void RemoteDebuggerPeerWeb::handle_payload(int p_handle, const uint8_t *p_data, int p_length) {
	RemoteDebuggerPeerWeb *peer = nullptr;
	{
		MutexLock lock(peer_map_mutex);
		RemoteDebuggerPeerWeb **ptr = peer_map.getptr(p_handle);
		if (ptr) {
			peer = *ptr;
		}
	}

	if (!peer) {
		return;
	}

	peer->_push_packet(p_data, p_length);
}

bool RemoteDebuggerPeerWeb::is_peer_connected() {
	MutexLock lock(mutex);
	return connected;
}

int RemoteDebuggerPeerWeb::get_max_message_size() const {
	return 8 << 20;
}

bool RemoteDebuggerPeerWeb::has_message() {
	MutexLock lock(mutex);
	return in_queue.size() > 0;
}

Array RemoteDebuggerPeerWeb::get_message() {
	MutexLock lock(mutex);
	ERR_FAIL_COND_V(in_queue.is_empty(), Array());
	Array msg = in_queue.front()->get();
	in_queue.pop_front();
	return msg;
}

Error RemoteDebuggerPeerWeb::put_message(const Array &p_arr) {
	if (handle == 0) {
		return ERR_UNCONFIGURED;
	}

	Variant payload = p_arr;
	int size = 0;
	Error err = encode_variant(payload, nullptr, size);
	ERR_FAIL_COND_V(err != OK, err);
	ERR_FAIL_COND_V(size > get_max_message_size(), ERR_OUT_OF_MEMORY);

	Vector<uint8_t> buffer;
	buffer.resize(size);
	encode_variant(payload, buffer.ptrw(), size);

	int send_err = godot_js_live_debug_send(handle, buffer.ptr(), buffer.size());
	ERR_FAIL_COND_V_MSG(send_err != 0, FAILED, "Failed to send debugger packet through web transport.");
	return OK;
}

void RemoteDebuggerPeerWeb::poll() {
	// Nothing to do, the JS glue pushes messages to us.
}

void RemoteDebuggerPeerWeb::close() {
	if (handle == 0) {
		return;
	}

	godot_js_live_debug_unregister_peer(handle);

	{
		MutexLock lock(peer_map_mutex);
		peer_map.erase(handle);
	}

	handle = 0;
	_set_connected(false);
}

extern "C" {

EMSCRIPTEN_KEEPALIVE void godot_js_live_debug_on_event(int p_handle, int p_event) {
	RemoteDebuggerPeerWeb::handle_event(p_handle, p_event);
}

EMSCRIPTEN_KEEPALIVE void godot_js_live_debug_on_payload(int p_handle, const uint8_t *p_data, int p_length) {
	RemoteDebuggerPeerWeb::handle_payload(p_handle, p_data, p_length);
}

} // extern "C"
