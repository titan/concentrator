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
#include "CINISection.h"

/**
* Constructor
* @param section The section to copy
*/
CINISection::CINISection(const CINISection & section)
{
	this->m_name = section.m_name;
	Copy(section);
}

/**
* Constructor
* @param name The section name
*/
CINISection::CINISection(const std::string & name)
{
	this->m_type = INI_ELEMENT_SECTION;
	SetName(name);
}

/**
* Destructor
*/
CINISection::~CINISection()
{
	Clear();
}

/**
* Set the name
* @param name The name
*/
void CINISection::SetName(const std::string & name)
{
	this->m_name = name;
}

/**
* Add an element
* @param element The element
*/
void CINISection::AddElement(CINIElement * element)
{
	this->m_data.push_back(element);
}

/**
* Copy the content of a section
* @param section The section to copy
*/
void CINISection::Copy(const CINISection & section)
{
	CINIProperty * tmp;
	CINIProperty * tmp2;

	for(TSectionElements::const_iterator it = section.m_data.begin() ; it != section.m_data.end() ; ++it)
	{
		switch((*it)->GetType())
		{
		case INI_ELEMENT_COMMENT:
			this->m_data.push_back(new CINIComment((*(CINIComment*)(*it))));
			break;

		case INI_ELEMENT_PROPERTY:
			// Get the current and the futur property
			tmp = (CINIProperty*)(*it);
			tmp2 = GetProperty(tmp->GetKey());

			// If the property already exists, it need to be modified
			if(tmp2 != NULL)
			{
				tmp2->SetValue(tmp->GetValue());

				// Only replace when a comment exists in the copied property
				if(tmp->GetComment() != "")
					tmp->SetComment(tmp->GetComment());
			}
			else
				// If no property exists, simply create it
				this->m_data.push_back(new CINIProperty((*(CINIProperty*)(*it))));

			break;

		default:
			break;
		}
	}
}

/**
* Remove all comments in the section
*/
void CINISection::RemoveAllComments()
{
	for(TSectionElements::iterator it = this->m_data.begin() ; it != this->m_data.end() ; )
	{
		switch((*it)->GetType())
		{
		case INI_ELEMENT_COMMENT:
			// The element is a comment, we remove it
			delete *it;
			it = this->m_data.erase(it);
			break;

		case INI_ELEMENT_PROPERTY:
			// The element is a property, we reset the comment
			static_cast<CINIProperty*>(*it)->SetComment("");
			++it;
			break;

		default:
			++it;
			break;
		}
	}
}

/**
* = operator overrides
* @param section A section
* @return The section
*/
const CINISection & CINISection::operator=(const CINISection & section)
{
	Clear();
	Copy(section);

	return *this;
}

/**
* Get the elements
* @return The elements
*/
const TSectionElements & CINISection::GetElements() const
{
	return this->m_data;
}

/**
* Get the name
* @return The name
*/
const std::string & CINISection::GetName() const
{
	return this->m_name;
}

/**
* Return a string representation of the element
* @return A string representation of the element
*/
const std::string CINISection::ToString() const
{
	std::string result = "";
	if(this->m_name != "")
		result += "[" + this->m_name + "]\n";

	for(TSectionElements::const_iterator it = this->m_data.begin() ; it != this->m_data.end() ; ++it)
		result += (*it)->ToString() + "\n";
	result += "\n";

	return result;
}

/**
* Get a data in a given section
* @param key The data key or NULL
*/
CINIProperty * CINISection::GetProperty(const std::string & key) const
{
	CINIProperty * result = NULL;

	// Search for the key
	for(TSectionElements::const_iterator it = this->m_data.begin() ; it != this->m_data.end() ; ++it)
	{
		if((*it)->GetType() == INI_ELEMENT_PROPERTY)
		{
			CINIProperty * data = (CINIProperty*)(*it);
			if(data->GetKey() == key)
			{
				result = data;
				break;
			}
		}
	}

	return result;
}

/**
* Clear the content of the section
*/
void CINISection::Clear()
{
	for(TSectionElements::iterator it = this->m_data.begin() ; it != this->m_data.end() ; ++it)
		delete *it;
	this->m_data.clear();
}
