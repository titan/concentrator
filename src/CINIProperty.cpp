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

#include "CINIProperty.h"

/**
* Constructor
* @param key The key name
* @param value The value
*/
CINIProperty::CINIProperty(const std::string & key, const std::string & value)
{
    this->m_type = INI_ELEMENT_PROPERTY;
    this->m_key = key;
    SetValue(value);
}

/**
* Constructor
* @param key The key name
* @param value The value
* @param comment The comment
*/
CINIProperty::CINIProperty(const std::string & key, const std::string & value, const std::string & comment)
{
    this->m_type = INI_ELEMENT_PROPERTY;
    this->m_key = key;
    SetValue(value);
    SetComment(comment);
}

/**
* Set the value
* @param value The value
*/
void CINIProperty::SetValue(const std::string & value)
{
    this->m_value = value;
}

/**
* Set the comment
* @param comment The comment
*/
void CINIProperty::SetComment(const std::string & comment)
{
    this->m_comment = comment;
}

/**
* Get the key name
* @return The key name
*/
const std::string & CINIProperty::GetKey() const
{
    return this->m_key;
}

/**
* Get the value
* @return The value
*/
const std::string & CINIProperty::GetValue() const
{
    return this->m_value;
}

/**
* Get the comment
* @return The comment
*/
const std::string & CINIProperty::GetComment() const
{
    return this->m_comment;
}

/**
* Return a string representation of the element
* @return A string representation of the element
*/
const std::string CINIProperty::ToString() const
{
    std::string result = "";
    if (this->m_value != "") {
        result = this->m_key + " = " + this->m_value;
        if (this->m_comment != "")
            result += " " + this->m_comment;
    }

    return result;
}
