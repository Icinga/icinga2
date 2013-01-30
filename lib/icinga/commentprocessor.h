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

#ifndef COMMENTPROCESSOR_H
#define COMMENTPROCESSOR_H

namespace icinga
{

enum CommentType
{
	Comment_User = 1,
	Comment_Downtime = 2,
	Comment_Flapping = 3,
	Comment_Acknowledgement = 4
};

/**
 * Comment processor.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API CommentProcessor
{
public:
	static int GetNextCommentID(void);

	static String AddComment(const DynamicObject::Ptr& owner,
	    CommentType entryType, const String& author, const String& text,
	    double expireTime);

	static void RemoveAllComments(const DynamicObject::Ptr& owner);
	static void RemoveComment(const String& id);

	static String GetIDFromLegacyID(int id);
	static DynamicObject::Ptr GetOwnerByCommentID(const String& id);
	static Dictionary::Ptr GetCommentByID(const String& id);

	static void InvalidateCommentCache(void);
	static void ValidateCommentCache(void);

private:
	static int m_NextCommentID;

	static map<int, String> m_LegacyCommentCache;
	static map<String, DynamicObject::WeakPtr> m_CommentCache;
	static bool m_CommentCacheValid;

	CommentProcessor(void);

	static void AddCommentsToCache(const DynamicObject::Ptr& owner);
};

}

#endif /* DOWNTIMEPROCESSOR_H */
