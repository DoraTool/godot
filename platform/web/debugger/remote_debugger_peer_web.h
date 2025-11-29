/**************************************************************************/
/*  remote_debugger_peer_web.h                                           */
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

#ifndef REMOTE_DEBUGGER_PEER_WEB_H
#define REMOTE_DEBUGGER_PEER_WEB_H

#include "core/debugger/remote_debugger_peer.h"

#include "core/os/mutex.h"
#include "core/templates/hash_map.h"

class RemoteDebuggerPeerWeb : public RemoteDebuggerPeer {
private:
	static Mutex peer_map_mutex;
	static HashMap<int32_t, RemoteDebuggerPeerWeb *> peer_map;

	int handle = 0;
	bool connected = false;
	String channel;
	bool server_endpoint = false;
	Mutex mutex;
	List<Array> in_queue;

	void _set_connected(bool p_connected);
	void _push_packet(const uint8_t *p_data, int p_length);

public:
	enum PeerEvent {
		EVENT_CONNECTED = 1,
		EVENT_DISCONNECTED = 2,
	};

	static RemoteDebuggerPeer *create(const String &p_uri);
	static void handle_event(int p_handle, int p_event);
	static void handle_payload(int p_handle, const uint8_t *p_data, int p_length);

	bool is_peer_connected() override;
	int get_max_message_size() const override;
	bool has_message() override;
	Error put_message(const Array &p_arr) override;
	Array get_message() override;
	void poll() override;
	void close() override;
	bool can_block() const override { return false; }

	RemoteDebuggerPeerWeb(const String &p_channel, bool p_is_server);
	~RemoteDebuggerPeerWeb();
};

#endif // REMOTE_DEBUGGER_PEER_WEB_H
