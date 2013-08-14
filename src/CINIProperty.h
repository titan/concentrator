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

#ifndef __CINIPROPERTY_H
#define __CINIPROPERTY_H

#include <string>
#include "CINIElement.h"

/**
* The property class.
* A property is composed of
* - A key
* - A value
* - A comment (optional)
*/
class CINIProperty : public CINIElement
{
protected:
    std::string m_key;
    std::string m_value;
    std::string m_comment;

public:
    CINIProperty(const std::string & key, const std::string & value);
    CINIProperty(const std::string & key, const std::string & value, const std::string & comment);
    void SetValue(const std::string & value);
    void SetComment(const std::string & comment);
    const std::string & GetKey() const;
    const std::string & GetValue() const;
    const std::string & GetComment() const;
    const std::string ToString() const;
};

#endif
