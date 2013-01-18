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

#ifndef __CINIELEMENT_H
#define __CINIELEMENT_H

#include <string>

enum INI_element
{
	INI_ELEMENT_COMMENT,
	INI_ELEMENT_SECTION,
	INI_ELEMENT_PROPERTY
};

/**
* The basic class for all elements
*/
class CINIElement
{
protected:
	INI_element m_type;

public:
	virtual ~CINIElement();
	INI_element GetType() const;
	virtual const std::string ToString() const = 0;
};

#endif
