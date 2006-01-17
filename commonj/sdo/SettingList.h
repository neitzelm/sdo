/*
 *
 *  Copyright 2005 The Apache Software Foundation or its licensors, as applicable.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/* $Rev$ $Date$ */

#ifndef SETTINGLIST_H
#define SETTINGLIST_H



#include <vector>
#include "commonj/sdo/Setting.h"

namespace commonj{
namespace sdo{

typedef std::vector<Setting> SETTING_VECTOR;

/**  SettingList is a list of settings returned by a change summary
 */

class SettingList
{

public:
    SettingList(SETTING_VECTOR sl);
    SettingList();

    virtual ~SettingList();

    /**  [] is the index operator
     *
     * This gives access to elements of the list
     */

    virtual Setting& operator[] (int pos) const;

    /**  size() returns the number of settings
     *
     * This method returns the size of the list os settings found.
     * These may be creations, deletions or changes.
     */

    virtual int size () const;
    
    virtual void insert (unsigned int index, const Setting& d);
    virtual void append (const Setting& d);
    virtual void remove (unsigned int index);


private: 
    SETTING_VECTOR slist;
    SETTING_VECTOR getVec() const;

    void validateIndex(int index) const;
};

};
};

#endif