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

#ifndef __CINI_H
#define __CINI_H

#include <utility>
#include <string>
#include <vector>
#include <memory>
#include "CINIElement.h"
#include "CINISection.h"

class CINIProperty;

/**
* The Simple ini Parser main class
* Contains all the functions to open, read, modify and save the content of an ini file
*/
class CINI
{
protected:
	std::string m_filename;
	std::vector<CINISection*> m_data;

public:
	CINI();
	CINI(const CINI & sinip);
	CINI(const std::string & filename);
	~CINI();
	void Parse(const std::string & filename);
	void SetValueBool(const std::string & section, const std::string & key, const bool value);
	void SetValueInt(const std::string & section, const std::string & key, const int value);
	void SetValueFloat(const std::string & section, const std::string & key, const float value);
	void SetValueDouble(const std::string & section, const std::string & key, const double value);
	void SetValueString(const std::string & section, const std::string & key, const std::string & value);
	void Clear();
	void RemoveSection(const std::string & section);
	void RemoveAllComments();
	void Copy(const CINI & sinip);
	void Save();
	void SaveToFile(const std::string & filename);
	const CINI & operator=(const CINI & sinip);
	const bool GetValueBool(const std::string & section, const std::string & key, const bool defaultValue) const;
	const int GetValueInt(const std::string & section, const std::string & key, const int defaultValue) const;
	const float GetValueFloat(const std::string & section, const std::string & key, const float defaultValue) const;
	const double GetValueDouble(const std::string & section, const std::string & key, const double defaultValue) const;
	const std::string GetValueString(const std::string & section, const std::string & key, const std::string & defaultValue) const;

protected:
	const std::string Trim(const std::string & text) const;
	const std::string GetValue(const std::string & section, const std::string & key) const;
	CINISection * GetSection(const std::string & section) const;
	CINIProperty * GetProperty(CINISection * section, const std::string & key) const;
};

#endif
