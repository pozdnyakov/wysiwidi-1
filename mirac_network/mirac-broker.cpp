/*
 * This file is part of XXX
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <glib-unix.h>

#include "mirac-broker.hpp"

/* static C callback wrapper */
gboolean MiracBroker::send_cb (gint fd, GIOCondition condition, gpointer data_ptr)
{
    auto broker = reinterpret_cast<MiracBroker*> (data_ptr);
    return broker->send_cb(fd, condition);
}

/* static C callback wrapper */
gboolean MiracBroker::receive_cb (gint fd, GIOCondition condition, gpointer data_ptr)
{
    auto broker = reinterpret_cast<MiracBroker*> (data_ptr);
    return broker->receive_cb(fd, condition);
}

/* static C callback wrapper */
gboolean MiracBroker::listen_cb (gint fd, GIOCondition condition, gpointer data_ptr)
{
    auto broker = reinterpret_cast<MiracBroker*> (data_ptr);
    return broker->listen_cb(fd, condition);
}

/* static C callback wrapper */
gboolean MiracBroker::connect_cb (gint fd, GIOCondition condition, gpointer data_ptr)
{
    auto broker = reinterpret_cast<MiracBroker*> (data_ptr);
    return broker->connect_cb(fd, condition);
}

gboolean MiracBroker::send_cb (gint fd, GIOCondition condition)
{
    try {
        return connection_->Send() ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
    } catch (std::exception &x) {
        g_warning("exception: %s", x.what());
    }
    return G_SOURCE_REMOVE;
}


gboolean MiracBroker::receive_cb (gint fd, GIOCondition condition)
{
    std::string msg;
    if (connection_->Receive(msg)) {
      got_message (msg);
    }
    return G_SOURCE_CONTINUE;
}

gboolean MiracBroker::listen_cb (gint fd, GIOCondition condition)
{
    try {
        connection_.reset(network_->Accept());
        g_message("connection from: %s", connection_->GetPeerAddress().c_str());
        g_unix_fd_add(connection_->GetHandle(), G_IO_IN, receive_cb, this);
        on_connected();
    } catch (std::exception &x) {
        g_warning("exception: %s", x.what());
    }

    return G_SOURCE_CONTINUE;
}

gboolean MiracBroker::connect_cb (gint fd, GIOCondition condition)
{
    try {
        if (!network_->Connect(NULL, NULL))
            return G_SOURCE_CONTINUE;
        g_message("connection success to: %s", network_->GetPeerAddress().c_str());
        connection_.reset(network_.release());
        g_unix_fd_add(connection_->GetHandle(), G_IO_IN, receive_cb, this);
        on_connected();
    } catch (std::exception &x) {
        g_warning("exception: %s", x.what());
    }
    return G_SOURCE_REMOVE;
}

void MiracBroker::send(WFD::Message& message) const
{
     if (connection_ && !connection_->Send(message.to_string()))
         g_unix_fd_add(connection_->GetHandle(), G_IO_OUT, send_cb, (void*)this);
}

unsigned short MiracBroker::get_host_port() const
{
    return network_->GetHostPort();
}

std::string MiracBroker::get_peer_address() const
{
    return connection_->GetPeerAddress();
}

MiracBroker::MiracBroker (const std::string& listen_port)
{
    network_.reset (new MiracNetwork());

    network_->Bind(NULL, listen_port.c_str());
    g_unix_fd_add(network_->GetHandle(), G_IO_IN,
                  MiracBroker::listen_cb, this);
}

MiracBroker::MiracBroker(const std::string& peer_address, const std::string& peer_port)
{
    network_.reset(new MiracNetwork());

    if (network_->Connect(peer_address.c_str(), peer_port.c_str())) {
        g_unix_fd_add(network_->GetHandle(), G_IO_OUT, MiracBroker::send_cb, this);
    } else {
        g_unix_fd_add(network_->GetHandle(), G_IO_OUT, MiracBroker::connect_cb, this);
    }
}

MiracBroker::~MiracBroker ()
{
}

void MiracBroker::SendRTSPData(const std::string& data) {
    if (connection_ && !connection_->Send(data))
        g_unix_fd_add(connection_->GetHandle(), G_IO_OUT, send_cb, (void*)this);
}
