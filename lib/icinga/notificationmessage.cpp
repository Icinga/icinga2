/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "icinga/notificationmessage.h"

using namespace icinga;

String NotificationMessage::GetService(void) const
{
	String service;
	Get("service", &service);
	return service;
}

void NotificationMessage::SetService(const String& service)
{
	Set("service", service);
}

String NotificationMessage::GetUser(void) const
{
	String user;
	Get("user", &user);
	return user;
}

void NotificationMessage::SetUser(const String& user)
{
	Set("user", user);
}

NotificationType NotificationMessage::GetType(void) const
{
	long type;
	Get("type", &type);
	return static_cast<NotificationType>(type);
}

void NotificationMessage::SetType(NotificationType type)
{
	Set("type", type);
}

String NotificationMessage::GetAuthor(void) const
{
	String author;
	Get("author", &author);
	return author;
}

void NotificationMessage::SetAuthor(const String& author)
{
	Set("author", author);
}

String NotificationMessage::GetCommentText(void) const
{
	String comment_text;
	Get("comment_text", &comment_text);
	return comment_text;
}

void NotificationMessage::SetCommentText(const String& comment_text)
{
	Set("comment_text", comment_text);
}

Dictionary::Ptr NotificationMessage::GetCheckResult(void) const
{
	Dictionary::Ptr cr;
	Get("check_result", &cr);
	return cr;
}

void NotificationMessage::SetCheckResult(const Dictionary::Ptr& result)
{
	Set("check_result", result);
}

