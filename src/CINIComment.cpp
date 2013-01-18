/*
==============================================================================================================================================================

www.sourceforge.net/projects/sinip
Copyright Stephane Baudoux (www.stephane-baudoux.com)

This file is part of CINI.

CINI is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CINI is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CINI.  If not, see <http://www.gnu.org/licenses/>.

==============================================================================================================================================================
*/

#include "CINIComment.h"

/**
* Constructor
* @param comment The comment text
*/
CINIComment::CINIComment(const std::string & comment)
{
	this->m_type = INI_ELEMENT_COMMENT;
	SetComment(comment);
}

/**
* Set the comment
* @param comment The comment text
*/
void CINIComment::SetComment(const std::string & comment)
{
	this->m_comment = comment;
}

/**
* Return a string representation of the element
* @return A string representation of the element
*/
const std::string CINIComment::ToString() const
{
	return this->m_comment;
}
