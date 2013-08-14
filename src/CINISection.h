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

#ifndef __CINISECTION_H
#define __CINISECTION_H

#include <string>
#include <vector>
#include <memory>
#include "CINIProperty.h"

typedef std::vector<CINIElement*> TSectionElements;

/**
* The section class.
* A section is composed of
* - A name (except for the anonymous section)
* - A set of elements
*/
class CINISection : public CINIElement
{
protected:
    std::string m_name;
    TSectionElements m_data;

public:
    CINISection(const CINISection & section);
    CINISection(const std::string & name);
    ~CINISection();
    void SetName(const std::string & name);
    void AddElement(CINIElement * element);
    void Copy(const CINISection & section);
    void RemoveAllComments();
    const CINISection & operator=(const CINISection & section);
    const TSectionElements & GetElements() const;
    const std::string & GetName() const;
    const std::string ToString() const;
    CINIProperty * GetProperty(const std::string & key) const;

protected:
    void Clear();
};

#endif
