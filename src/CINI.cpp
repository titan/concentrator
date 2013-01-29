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

#include <algorithm>
#include <sstream>
#include <fstream>
#include "CINI.h"
#include "CINIProperty.h"
#include "CINIComment.h"

#ifdef DEBUG_INI
#define DEBUG printf
#else
#define DEBUG(...)
#endif

/**
* Constructor.
* Use the default filename 'new.ini'
*/
CINI::CINI()
{
	this->m_filename = "new.ini";
}

/**
* Constructor.
* @param sinip The ini file to copy
*/
CINI::CINI(const CINI & sinip)
{
	Copy(sinip);
}

/**
* Constructor.
* Open and parse the given file
* @param filename The ini file to parse
*/
CINI::CINI(const std::string & filename)
{
	Parse(filename);
}

/**
* Destructor
*/
CINI::~CINI()
{
	Clear();
}

/**
* Parse an ini file
* @param filename The ini file to parse
*/
void CINI::Parse(const std::string & filename)
{
   DEBUG("CINI::Parse(%s)\n", filename.c_str());
	this->m_data.clear();
	this->m_filename = filename;

	std::string line;
	std::string currentSection = "";

	std::string::size_type pos, pos2;

	// Anonymous section
	this->m_data.push_back(new CINISection(""));

	// Open the file
	std::fstream file(this->m_filename.c_str());
	if(file.is_open())
	{
      DEBUG("open %s OK\n", filename.c_str());
		// While there is no errors.
		while(file.good())
		{
			// Get a line.
			std::getline(file, line);
			line = Trim(line);

			if(line != "")
			{
				// Comment
				if(line[0] == '#' || line[0] == ';')
				{
					GetSection(currentSection)->AddElement(new CINIComment(line));
				}
				else
				{
					pos = line.find_first_of('[');
				    pos2=line.find_first_of(']');

				    // Section
				    if(pos != std::string::npos && pos2 != std::string::npos && pos < pos2)
				    {
				        currentSection = line.substr(pos + 1, pos2 - (pos + 1));
						this->m_data.push_back(new CINISection(currentSection));
				    }
					else
					{
						pos = line.find_first_of('=');
						if(pos != std::string::npos)
						{
							// Property
							pos2 = std::min(line.find_first_of(';'), line.find_first_of('#'));
							std::string key = Trim(line.substr(0, pos));
							std::string value;

							// Search for a comment
							if(pos2 != std::string::npos)
							{
								// Property with a comment
								value = Trim(line.substr(pos + 1, pos2 - (pos + 1)));

								if(key != "" && value != "")
									GetSection(currentSection)->AddElement(new CINIProperty(key, value, Trim(line.substr(pos2, line.length() - pos2))));
							}
							else
							{
								// Property without comment
								value = Trim(line.substr(pos + 1, line.length() - (pos + 1)));

								if(key != "" && value != "")
									GetSection(currentSection)->AddElement(new CINIProperty(key, value));
							}
						}
					}
				}
			}
		}

		file.close();
	}
}

/**
* Set a boolean value
* @param section The section name
* @param key The key name
* @param value The value
*/
void CINI::SetValueBool(const std::string & section, const std::string & key, const bool value)
{
	if(key != "")
	{
		std::ostringstream ss("");
		ss << value;
		SetValueString(section, key, ss.str());
	}
}

/**
* Set an integer value
* @param section The section name
* @param key The key name
* @param value The value
*/
void CINI::SetValueInt(const std::string & section, const std::string & key, const int value)
{
	if(key != "")
	{
		std::ostringstream ss("");
		ss << value;
		SetValueString(section, key, ss.str());
	}
}

/**
* Set a float value
* @param section The section name
* @param key The key name
* @param value The value
*/
void CINI::SetValueFloat(const std::string & section, const std::string & key, const float value)
{
	if(key != "")
	{
		std::ostringstream ss("");
		ss << value;
		SetValueString(section, key, ss.str());
	}
}

/**
* Set a double value
* @param section The section name
* @param key The key name
* @param value The value
*/
void CINI::SetValueDouble(const std::string & section, const std::string & key, const double value)
{
	if(key != "")
	{
		std::ostringstream ss("");
		ss << value;
		SetValueString(section, key, ss.str());
	}
}

/**
* Set a string value
* @param section The section name
* @param key The key name
* @param value The value
*/
void CINI::SetValueString(const std::string & section, const std::string & key, const std::string & value)
{
	if(key != "")
	{
		CINISection * sectionPtr = GetSection(section);

		// If the section is not found, it need to be created
		if(sectionPtr == NULL)
		{
			sectionPtr = new CINISection(section);
			this->m_data.push_back(sectionPtr);
		}

		// Create or modify the property
		CINIProperty * data = GetProperty(sectionPtr, key);
		if(data == NULL)
			sectionPtr->AddElement(new CINIProperty(key, value));
		else
			data->SetValue(value);
	}
}

/**
* Clear the ini file
*/
void CINI::Clear()
{
	for(std::vector<CINISection*>::iterator it = this->m_data.begin() ; it != this->m_data.end() ; ++it)
		delete *it;
	this->m_data.clear();
}

/**
* Remove a section from the ini file
* @param section The section name
*/
void CINI::RemoveSection(const std::string & section)
{
	for(std::vector<CINISection*>::iterator it = this->m_data.begin() ; it != this->m_data.end() ; ++it)
	{
		if((*it)->GetName() == section)
		{
			delete *it;
			this->m_data.erase(it);
			break;
		}
	}
}

/**
* Remove all the comments in the ini file
*/
void CINI::RemoveAllComments()
{
	for(std::vector<CINISection*>::iterator it = this->m_data.begin() ; it != this->m_data.end() ; ++it)
		(*it)->RemoveAllComments();
}

/**
* Copy the content of the ini file
* @param sinip The ini file
*/
void CINI::Copy(const CINI & sinip)
{
	for(std::vector<CINISection*>::const_iterator it = sinip.m_data.begin() ; it != sinip.m_data.end() ; ++it)
	{
		CINISection * tmp = GetSection((*it)->GetName());

		// If the section already exists, it need to be modified
		if(tmp != NULL)
			tmp->Copy(**it);
		else
			// If no section is found, simply create it
			this->m_data.push_back(new CINISection(**it));
	}
}

/**
* Save the ini file
*/
void CINI::Save()
{
	SaveToFile(this->m_filename);
}

/**
* Save the configuration in a given file
* @param filename The filename
*/
void CINI::SaveToFile(const std::string & filename)
{
#if 0
	std::ofstream file(filename);
	for(std::vector<CINISection*>::const_iterator it = this->m_data.begin() ; it != this->m_data.end() ; ++it)
		file << (*it)->ToString();
	file.close();
#endif
}

/**
* = operator overrides
* @param sinip A sinip class
* @return The sinip class
*/
const CINI & CINI::operator=(const CINI & sinip)
{
	Clear();
	Copy(sinip);

	return *this;
}

/**
* Get a boolean value
* @param section The section name
* @param key The key name
* @param defaultValue The default value to use if no key is found
* @return The found value or the default one
*/
const bool CINI::GetValueBool(const std::string & section, const std::string & key, const bool defaultValue) const
{
	bool result = defaultValue;
	std::string value = GetValue(section, key);

	if(value != "")
	{
		std::transform(value.begin(), value.end(), value.begin(), tolower);
		if(value == "0" || value == "false")
		{
			result = false;
		}
		else
		{
			if(value == "1" || value == "true")
				result = true;
		}
	}

	return result;
}

/**
* Get an integer value
* @param section The section name
* @param key The key name
* @param defaultValue The default value to use if no key is found
* @return The found value or the default one
*/
const int CINI::GetValueInt(const std::string & section, const std::string & key, const int defaultValue) const
{
	int result = defaultValue;
	std::string value = GetValue(section, key);

	// Try to convert the data into the wanted format
	if(value != "")
	{
		std::istringstream ss(value);
		ss >> result;
	}

	return result;
}

/**
* Get a float value
* @param section The section name
* @param key The key name
* @param defaultValue The default value to use if no key is found
* @return The found value or the default one
*/
const float CINI::GetValueFloat(const std::string & section, const std::string & key, const float defaultValue) const
{
	float result = defaultValue;
	std::string value = GetValue(section, key);

	// Try to convert the data into the wanted format
	if(value != "")
	{
		std::istringstream ss(value);
		ss >> result;
	}

	return result;
}

/**
* Get a double value
* @param section The section name
* @param key The key name
* @param defaultValue The default value to use if no key is found
* @return The found value or the default one
*/
const double CINI::GetValueDouble(const std::string & section, const std::string & key, const double defaultValue) const
{
	double result = defaultValue;
	std::string value = GetValue(section, key);

	// Try to convert the data into the wanted format
	if(value != "")
	{
		std::istringstream ss(value);
		ss >> result;
	}

	return result;
}

/**
* Get a string value
* @param section The section name
* @param key The key name
* @param defaultValue The default value to use if no key is found
* @return The found value or the default one
*/
const std::string CINI::GetValueString(const std::string & section, const std::string & key, const std::string & defaultValue) const
{
	std::string result = GetValue(section, key);
	if(result == "")
		result = defaultValue;

	return result;
}

/**
* Trim a string (remove all ' ' before and after a text)
* @param text The text
* @return The modified text
*/
const std::string CINI::Trim(const std::string & text) const
{
	std::string result = text;

	// Trim the beginning of the text
	std::string::size_type pos = text.find_last_not_of(' ');
	if(pos != std::string::npos)
		result.erase(pos + 1);

	// Trim the ending of the text
	pos = text.find_first_not_of(' ');
	if(pos != std::string::npos)
		result.erase(0, pos);

	return result;
}

/**
* Get a value
* @param section The section name
* @param key The key name
* @return The value or ' ' if no value is found
*/
const std::string CINI::GetValue(const std::string & section, const std::string & key) const
{
	std::string result = "";
	CINISection * sectionPtr = GetSection(section);

	// If the section exists
	if(sectionPtr != NULL)
	{
		// Find a property with the given key name
		for(TSectionElements::const_iterator it = sectionPtr->GetElements().begin() ; it != sectionPtr->GetElements().end() ; ++it)
		{
			if((*it)->GetType() == INI_ELEMENT_PROPERTY)
			{
				CINIProperty * tmp = (CINIProperty*)(*it);
				if(tmp->GetKey() == key && tmp->GetValue() != "")
				{
					result = tmp->GetValue();
					break;
				}
			}
		}
	}

	return result;
}

/**
* Get the section
* @param section The section name
* @return The section or NULL
*/
CINISection * CINI::GetSection(const std::string & section) const
{
	CINISection * result = NULL;
	for(unsigned int i = 0 ; i < this->m_data.size() ; ++i)
	{
		if(this->m_data.at(i)->GetName() == section)
		{
			result = this->m_data.at(i);
			break;
		}
	}

	return result;
}

/**
* Get a data in a given section
* @param section The section to search into
* @param key The data key or NULL
*/
CINIProperty * CINI::GetProperty(CINISection * section, const std::string & key) const
{
	if(section == NULL)
		return NULL;

	return section->GetProperty(key);
}
