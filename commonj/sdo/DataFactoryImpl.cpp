/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 *   
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/* $Rev: 547318 $ $Date$ */

//////////////////////////////////////////////////////////////////////
// DataFactoryImpl.cpp: implementation of the DataFactory class.
//
//////////////////////////////////////////////////////////////////////
#include "commonj/sdo/SDORuntimeException.h"

#include "commonj/sdo/DataFactoryImpl.h"
#include "commonj/sdo/DataObjectImpl.h"

#include "commonj/sdo/Logger.h"

#include "commonj/sdo/SDOUtils.h"

#include "commonj/sdo/PropertyList.h"

#include <iostream>
#include <stdio.h>


using namespace std;
using namespace commonj::sdo;

namespace commonj{
namespace sdo {

// ===================================================================
// Constructor
// ===================================================================
DataFactoryImpl::DataFactoryImpl()
{

    /* add the primitives to every mdg - */

    isResolved = false;
    addType(Type::SDOTypeNamespaceURI,"BigDecimal");
    addType(Type::SDOTypeNamespaceURI,"BigInteger");
    addType(Type::SDOTypeNamespaceURI,"Boolean");

    addType(Type::SDOTypeNamespaceURI,"Byte");
    addType(Type::SDOTypeNamespaceURI,"Bytes");
    addType(Type::SDOTypeNamespaceURI,"Character");

    addType(Type::SDOTypeNamespaceURI,"String");
    addType(Type::SDOTypeNamespaceURI,"DataObject");
    addType(Type::SDOTypeNamespaceURI,"OpenDataObject", true, true, false, false);
    addType(Type::SDOTypeNamespaceURI,"Date");
    addType(Type::SDOTypeNamespaceURI,"Double");
    addType(Type::SDOTypeNamespaceURI,"Float");
    addType(Type::SDOTypeNamespaceURI,"Integer");
    addType(Type::SDOTypeNamespaceURI,"Long");
    addType(Type::SDOTypeNamespaceURI,"Short");
    addType(Type::SDOTypeNamespaceURI,"URI");

    // abstract
    addType(Type::SDOTypeNamespaceURI,"ChangeSummary");

}

// ===================================================================
// Destructor
// ===================================================================
DataFactoryImpl::~DataFactoryImpl()
{

   resolvePending.clear();
   
    TYPES_MAP::iterator typeIter;
    for (typeIter = types.begin() ; typeIter != types.end() ; ++typeIter)
    {
        if (strncmp((typeIter->first).c_str(),"ALIAS::", 7)) 
        {
            delete typeIter->second;
        }
    }
}

// ===================================================================
// copy constructor
// ===================================================================
DataFactoryImpl::DataFactoryImpl(const DataFactoryImpl& inmdg)
{
    isResolved = false;
    copyTypes(inmdg);
}

// ===================================================================
// Assignment operator
// ===================================================================
DataFactoryImpl& DataFactoryImpl::operator=(const DataFactoryImpl& inmdg)
{
    if (this != &inmdg)
    {
        copyTypes(inmdg);
    }
    return *this;
}

// ===================================================================
// copy Types to this DataFactory
// ===================================================================
void DataFactoryImpl::copyTypes(const DataFactoryImpl& inmdg)
{


    TYPES_MAP::const_iterator typeIter;
    TYPES_MAP::iterator typeIter2;
    SDOString fullTypeName;

    for (typeIter = inmdg.types.begin() ; typeIter != inmdg.types.end() ; ++typeIter)
    {
        // add this type to this metadata
        addType((typeIter->second)->getURI(), (typeIter->second)->getName());

        // re-find the type we just added.
        getFullTypeName(fullTypeName,
                (typeIter->second)->getURI(), 
                (typeIter->second)->getName());
        typeIter2 = types.find(fullTypeName);

        // copy the aliases , if there are any.

        if ((typeIter->second)->getAliasCount() > 0)
        {
            for (unsigned int j=0;j<(typeIter->second)->getAliasCount();j++)
            {
                (typeIter2->second)->setAlias(
                    (typeIter->second)->getAlias());
            }
        }

        
        // Now add all the properties
        const std::list<PropertyImpl*> props = typeIter->second->getPropertyListReference();

        for (std::list<PropertyImpl*>::const_iterator i = props.begin();
             i != props.end();
             i++)
        {
            // Ensure the properties type is added
            const Type& propType = (*i)->getType();
            addType(propType.getURI(), propType.getName());

            // Now add the property
            addPropertyToType((typeIter->second)->getURI(),
                              (typeIter->second)->getName(),
                              (*i)->getName(),
                              propType.getURI(), 
                              propType.getName(),
                              (*i)->isMany(),
                              (*i)->isReadOnly(),
                              (*i)->isContainment());

            // copy the aliases if there are any.
            if ((*i)->getAliasCount() > 0) 
            {

                PropertyImpl* p = (typeIter2->second)->
                    getPropertyImpl((*i)->getName());
                if (p != 0)
                {
                    for (unsigned int j=0;j<p->getAliasCount();j++)
                    {
                        p->setAlias((*i)->getAlias(j));
                    }
                }

            }

        } // end - iterate over Properties
    } // end - iterate over Types
}

// ===================================================================
// addType - adds a new Type if it does not already exist
// ===================================================================
void DataFactoryImpl::addType(const char* uri, const char* inTypeName, 
                                bool isSeq,
                                bool isOp,
                                bool isAbs,
                                bool isData)
{
    addType(uri,inTypeName,isSeq,isOp,isAbs,isData,false);
}

void DataFactoryImpl::addType(const SDOString& uri, const SDOString& inTypeName, 
                                bool isSeq,
                                bool isOp,
                                bool isAbs,
                                bool isData)
{
    addType(uri.c_str(),inTypeName.c_str(),isSeq,isOp,isAbs,isData,false);
}

bool empty(const char* inTypeName)
{
    return inTypeName == 0 || strlen(inTypeName) == 0;
}
void assertTypeName(const char* inTypeName, const char* function)
{
    if (empty(inTypeName))
    {
        SDO_THROW_EXCEPTION(function,
            SDOIllegalArgumentException, " Type has empty name");
    }
}

void DataFactoryImpl::addType(const char* uri, const char* inTypeName, 
                                bool isSeq,
                                bool isOp,
                                bool isAbs,
                                bool isData,
                                bool isFromList)
{
    assertTypeName(inTypeName, "DataFactory::addType");

    SDOString typeUri;

    if (uri)
        typeUri = uri;
    if (findType(typeUri, inTypeName) == 0) 
    {
        SDOString fullTypeName;
		getFullTypeName(fullTypeName, uri, inTypeName);
        types[fullTypeName] = new TypeImpl(uri, inTypeName, isSeq, isOp, isAbs, isData, isFromList);
        resolvePending[fullTypeName] = types[fullTypeName];
    }
}

void DataFactoryImpl::addType(const SDOString& uri, const SDOString& inTypeName, 
                                bool isSeq,
                                bool isOp,
                                bool isAbs,
                                bool isData,
                                bool isFromList)
{
    addType(uri.c_str(), inTypeName.c_str(), isSeq, isOp, isAbs, isData, isFromList);
}

// ===================================================================
//  Check whether a change summary would clash.
// ===================================================================

bool DataFactoryImpl::recursiveCheck(TypeImpl* cs, TypeImpl* t)
{
    if (cs->isDataType()) return false;

    if (! strcmp(cs->getName(), t->getName()) &&
        ! strcmp(cs->getURI() , t->getURI()) )
    {
        return true;
    }

    const std::list<PropertyImpl*> pl = cs->getPropertyListReference();
    
    for (std::list<PropertyImpl*>::const_iterator j = pl.begin();
         j != pl.end();
         j++)
    {
        if (recursiveCheck((TypeImpl*)&((*j)->getType()), t)) return true;
    }
    return false;
}

// ===================================================================
//  Check whether a change summary would clash.
// ===================================================================
bool DataFactoryImpl::checkForValidChangeSummary(TypeImpl* t)
{
    // None of the containing types can have a cs already.
    // None of the properties of this type can hold a type
    // which has a change summary.

   if (t->isResolved)
    {
    SDO_THROW_EXCEPTION("DataFactory::checkForValidChangeSummary",
        SDOUnsupportedOperationException, "Adding Change Summary after type completed");
    }

    if (cstypes.size() > 0) {
        for (unsigned int i = 0 ;i < cstypes.size(); i++) 
        {
            if (recursiveCheck(cstypes[i], t)) 
            {
                return false;

            }
        }
    }
    cstypes.push_back(t);
    return true;
}

const char*const addPropertyToType = "DataFactory::addPropertyToType";
void assertNames(const char* inTypeName, const char* propname)
{
    assertTypeName(inTypeName, addPropertyToType);
    if (empty(propname))
    {
        SDO_THROW_EXCEPTION(addPropertyToType,
            SDOIllegalArgumentException, " Property has an empty name");
    }
}

// ===================================================================
// addPropertyToType - adds a Property to an existing Type
// ===================================================================
void DataFactoryImpl::addPropertyToType(const char* uri, 
                                      const char* inTypeName, 
                                      const char* propname,
                                      const char* propTypeUri,
                                      const char* propTypeName,
                                      bool    many)
{
    assertNames(inTypeName, propname);
    SDOString fullPropTypeName;
    getFullTypeName(fullPropTypeName, propTypeUri, propTypeName);

    TYPES_MAP::iterator typeIter;
    typeIter = types.find(fullPropTypeName);
    if (typeIter != types.end())
    {
        addPropertyToType(uri,inTypeName, 
                                  propname,
                                  propTypeUri,
                                  propTypeName,
                                  many, 
                                  false,
                                  !(typeIter->second)->isDataType());
    }
}

void DataFactoryImpl::addPropertyToType(const SDOString& uri,
                                        const SDOString& inTypeName,
                                        const SDOString& propname,
                                        const SDOString& propTypeUri,
                                        const SDOString& propTypeName,
                                        bool many)
{
  addPropertyToType(uri.c_str(), inTypeName.c_str(), propname.c_str(), propTypeUri.c_str(), propTypeName.c_str(), many);
}

void DataFactoryImpl::addPropertyToType(const char* uri, 
                                      const char* inTypeName, 
                                      const char* propname,
                                      const char* propTypeUri,
                                      const char* propTypeName,
                                      bool    many,
                                      bool    rdonly,
                                      bool cont)
{
    assertNames(inTypeName, propname);

    TYPES_MAP::iterator typeIter, typeIter2;
    
    //cout << "Adding property " << propname << "(" << propTypeUri << "#" << propTypeName
    //    << ") to type " << uri << "#" << inTypeName << endl;


    SDOString fullTypeName;
	getFullTypeName(fullTypeName, uri, inTypeName);
    typeIter = types.find(fullTypeName);

    if(typeIter == types.end())
    {
        string msg("Type not found: ");
        if (uri != 0)msg += uri;
        msg += " ";
        if (inTypeName != 0)msg += inTypeName;
        SDO_THROW_EXCEPTION("addPropertyToType",
        SDOTypeNotFoundException, msg.c_str());

    }

    if ((typeIter->second)->isDataType())
    {
        string msg("Cannot add property ");
        msg += propname;
        msg += " of type ";
        msg += propTypeUri;
        msg += "#";
        msg += propTypeName;
        msg += " to this data type ";
        msg += uri;
        msg += "#";
        msg += inTypeName;
        SDO_THROW_EXCEPTION("addPropertyToType",
        SDOIllegalArgumentException, msg.c_str());
    }

    getFullTypeName(fullTypeName, propTypeUri, propTypeName);
    typeIter2 = types.find(fullTypeName);
    
    if (typeIter2 == types.end())
    {
        string msg("Type not found: ");
        if (propTypeUri != 0)msg += propTypeUri;
        msg += " ";
        if (propTypeName != 0)msg += propTypeName;
        SDO_THROW_EXCEPTION("addPropertyToType",
        SDOTypeNotFoundException, msg.c_str());
    }
    
    // Check if its a ChangeSummary
    if (propTypeUri != 0 && !strcmp(propTypeUri,Type::SDOTypeNamespaceURI.c_str()) &&
        !strcmp(propTypeName,"ChangeSummary") )
    {
        if (checkForValidChangeSummary(typeIter->second)) 
        {
            // The change summary is allowable if we got to here - force the right params.
            // we will not use this property - its just for compatibility.
            // we have to use getChangeSummary to get the change summary, 
            // and isChangeSummaryType to see if this is a type which may have
            // a change summary.
            (typeIter->second)->addChangeSummary();
            // dont even show the property - its not needed
            //((typeIter->second)->addProperty(propname, *(typeIter2->second),false,true, false));

        }
        return;
    }
    

    if ((typeIter->second)->isDataType())
    {
        string msg("Cannot add property to a data type : ");
        msg += (typeIter->second)->getName();
        SDO_THROW_EXCEPTION("addPropertyToType",
        SDOIllegalArgumentException, msg.c_str());
           // cannot add a property to a primitive
    }

    // @PGR@ containment should be ignored for DataType
/*    if ((typeIter2->second)->isDataType() && cont == true)
    {
        string msg("Data types may not be containment : ");
        msg += (typeIter2->second)->getName();
        SDO_THROW_EXCEPTION("addPropertyToType",
        SDOIllegalArgumentException, msg.c_str());
        // cannot try to make a property containment on a data type
    }
*/

    if ((typeIter->second)->isResolved)
    {
    SDO_THROW_EXCEPTION("DataFactory::addPropertyToType",
        SDOUnsupportedOperationException, "Adding Properties after type completed");
    }

    ((typeIter->second)->addProperty(propname, *(typeIter2->second),many,rdonly, cont));
    return;
}

void DataFactoryImpl::addPropertyToType(const SDOString& uri,
                                        const SDOString& inTypeName,
                                        const SDOString& propname,
                                        const SDOString& propTypeUri,
                                        const SDOString& propTypeName,
                                        bool many,
                                        bool rdonly,
                                        bool cont)
{
  addPropertyToType(uri.c_str(),
                           inTypeName.c_str(),
                           propname.c_str(),
                           propTypeUri.c_str(),
                           propTypeName.c_str(),
                           many,
                           rdonly,
                           cont);
}

// ===================================================================
// addPropertyToType - adds a Property to an existing Type
// ===================================================================

void DataFactoryImpl::addPropertyToType(const char* uri, 
                                      const char* inTypeName, 
                                      const char* propname,
                                      const Type& tprop,
                                      bool    many)
{
    addPropertyToType(uri, 
                    inTypeName, 
                    propname,
                    tprop,
                    many,
                    false,
                    !tprop.isDataType());
}

void DataFactoryImpl::addPropertyToType(const SDOString& uri, 
                                        const SDOString& inTypeName, 
                                        const SDOString& propname,
                                        const Type& tprop,
                                        bool many)
{
  addPropertyToType(uri.c_str(), 
                    inTypeName.c_str(), 
                    propname.c_str(),
                    tprop,
                    many,
                    false,
                    !tprop.isDataType());
}

void DataFactoryImpl::addPropertyToType(const char* uri, 
                                      const char* inTypeName, 
                                      const char* propname,
                                      const Type& tprop,
                                      bool    many,
                                      bool    rdonly,
                                      bool cont)
{
    addPropertyToType(uri, 
                      inTypeName,
                      propname,
                      tprop.getURI(),
                      tprop.getName(),
                      many,
                      rdonly, cont);
}

void DataFactoryImpl::addPropertyToType(const SDOString& uri, 
                                        const SDOString& inTypeName, 
                                        const SDOString& propname,
                                        const Type& tprop,
                                        bool many,
                                        bool rdonly,
                                        bool cont)
{
  addPropertyToType(uri.c_str(), 
                    inTypeName.c_str(),
                    propname.c_str(),
                    tprop.getURI(),
                    tprop.getName(),
                    many,
                    rdonly,
                    cont);
}

// ===================================================================
// addPropertyToType - adds a Property to an existing Type
// ===================================================================
void DataFactoryImpl::addPropertyToType(const Type& cont,
                                      const char* propname,
                                      const char* propTypeUri,
                                      const char* propTypeName,
                                      bool  many)
{
    addPropertyToType(cont.getURI(),
                      cont.getName(),
                      propname,
                      propTypeUri,
                      propTypeName,
                      many);
}

void DataFactoryImpl::addPropertyToType(const Type& cont,
                                        const SDOString& propname,
                                        const SDOString& propTypeUri,
                                        const SDOString& propTypeName,
                                        bool many)
{
  addPropertyToType(cont.getURI(),
                    cont.getName(),
                    propname.c_str(),
                    propTypeUri.c_str(),
                    propTypeName.c_str(),
                    many);
}

void DataFactoryImpl::addPropertyToType(const Type& cont,
                                      const char* propname,
                                      const char* propTypeUri,
                                      const char* propTypeName,
                                      bool  many,
                                      bool  rdonly,
                                      bool contain)
{
    addPropertyToType(cont.getURI(),
                      cont.getName(),
                      propname,
                      propTypeUri,
                      propTypeName,
                      many,
                      rdonly,
                      contain);
}

void DataFactoryImpl::addPropertyToType(const Type& cont,
                                      const SDOString& propname,
                                      const SDOString& propTypeUri,
                                      const SDOString& propTypeName,
                                      bool many,
                                      bool rdonly,
                                      bool contain)
{
    addPropertyToType(cont.getURI(),
                      cont.getName(),
                      propname.c_str(),
                      propTypeUri.c_str(),
                      propTypeName.c_str(),
                      many,
                      rdonly,
                      contain);
}

// ===================================================================
// addPropertyToType - adds a Property to an existing Type
// ===================================================================
void DataFactoryImpl::addPropertyToType(const Type& tp,
                                      const char* propname,
                                      const Type& tprop,
                                      bool  many)
{
        addPropertyToType(tp.getURI(),
                      tp.getName(),
                      propname,
                      tprop.getURI(),
                      tprop.getName(),
                      many);
}

void DataFactoryImpl::addPropertyToType(const Type& tp,
                                        const SDOString& propname,
                                        const Type& tprop,
                                        bool many)
{
  addPropertyToType(tp.getURI(),
                    tp.getName(),
                    propname.c_str(),
                    tprop.getURI(),
                    tprop.getName(),
                    many);
}

void DataFactoryImpl::addPropertyToType(const Type& tp,
                                      const char* propname,
                                      const Type& tprop,
                                      bool  many,
                                      bool  rdonly,
                                      bool cont)
{
    addPropertyToType(tp.getURI(),
                      tp.getName(),
                      propname,
                      tprop.getURI(),
                      tprop.getName(),
                      many,
                      rdonly,
                      cont);
}

void DataFactoryImpl::addPropertyToType(const Type& tp,
                                        const SDOString& propname,
                                        const Type& tprop,
                                        bool many,
                                        bool rdonly,
                                        bool cont)
{
  addPropertyToType(tp.getURI(),
                    tp.getName(),
                    propname.c_str(),
                    tprop.getURI(),
                    tprop.getName(),
                    many,
                    rdonly,
                    cont);
}

// ===================================================================
// getFullTypeName - return the name used as a key in the types map
// ===================================================================
SDOString& DataFactoryImpl::getFullTypeName(SDOString& stringBuffer, const SDOString& uri, const SDOString& inTypeName)
{
   stringBuffer = uri;
   stringBuffer += "#";
   stringBuffer += inTypeName;
   return stringBuffer;
}

// ===================================================================
// getAliasTypeName - return the name used as a key in the types map
// ===================================================================
SDOString& DataFactoryImpl::getAliasTypeName(SDOString& stringBuffer, const SDOString& uri, const SDOString& inTypeName)
{
  getFullTypeName(stringBuffer, uri, inTypeName);
  stringBuffer.insert(0, "ALIAS::");
  return stringBuffer;
}

// ===================================================================
// getType - return a pointer to the required Type
// ===================================================================
const Type& DataFactoryImpl::getType(const char* uri, const char* inTypeName) const
{

    const Type* type = findType(uri, inTypeName);

    if (type == 0)
    {
        string msg("Type not found :");
        if (uri != 0) msg += uri;
        msg += " ";
        if (inTypeName != 0) msg += inTypeName;
        SDO_THROW_EXCEPTION("getType" ,
        SDOTypeNotFoundException, msg.c_str());
    }
    
    return *type;
}

const Type& DataFactoryImpl::getType(const SDOString& uri, const SDOString& inTypeName) const
{
    return getType(uri.c_str(), inTypeName.c_str());
}

// ===================================================================
// setBaseType - sets the type from which this type inherits properties
// ===================================================================

void DataFactoryImpl::setBaseType( const Type& type,
                  const Type& base, bool isRestriction) 
{
    setBaseType(type.getURI(),type.getName(),base.getURI(), base.getName(),
        isRestriction);
}

// ===================================================================
// setBaseType - sets the type from which this type inherits properties
// ===================================================================

void DataFactoryImpl::setBaseType( const char* typeuri,
                  const char* typenam,
                  const char* baseuri,
                     const char* basename,
                     bool isRestriction )
{
    const TypeImpl* base = findTypeImpl(baseuri, basename);
    if (base == 0)
    {
        string msg("Type not found :");
        if (baseuri != 0) msg += baseuri;
        msg += " ";
        if (basename != 0) msg += basename;
        SDO_THROW_EXCEPTION("setBaseType" ,
        SDOTypeNotFoundException, msg.c_str());
    }

    TYPES_MAP::const_iterator typeIter;

    SDOString fullTypeName;
	getFullTypeName(fullTypeName, typeuri, typenam);
    typeIter = types.find(fullTypeName);
    
    if(typeIter == types.end())
    {
        string msg("Type not found :");
        if (typeuri != 0) msg += typeuri;
        msg += " ";
        if (typenam != 0) msg += typenam;
        SDO_THROW_EXCEPTION("setBaseType" ,
        SDOTypeNotFoundException, msg.c_str());
    }

    (typeIter->second)->setBaseType(base, isRestriction);
}

void DataFactoryImpl::setBaseType(const SDOString& typeuri,
                                  const SDOString& typenam,
                                  const SDOString& baseuri,
                                  const SDOString& basename,
                                  bool isRestriction)
{
    setBaseType(typeuri.c_str(),
        typenam.c_str(),
        baseuri.c_str(),
        basename.c_str(),
        isRestriction);
}


// ===================================================================
// setPropertySubstitute - additional type for a property
// ===================================================================

    void DataFactoryImpl::setPropertySubstitute(
            const char* uri, 
            const char* inTypeName,
            const char* propname,
            const char* subname,
            const char* subTypeUri, 
            const char* subTypeName)
    {
        const TypeImpl* cont = findTypeImpl(uri, inTypeName);
        if (cont == 0)
        {
            string msg("Type not found :");
            if (uri != 0) msg += uri;
            msg += " ";
            if (inTypeName != 0) msg += inTypeName;
            SDO_THROW_EXCEPTION("setPropertySubstitute" ,
            SDOTypeNotFoundException, msg.c_str());
        }
        PropertyImpl* pi = cont->getPropertyImpl(propname);
        const Type& tsub = getType(subTypeUri,
                                subTypeName);
        if (pi != 0)pi->setSubstitution(this,subname,tsub);
    }

    void DataFactoryImpl::setPropertySubstitute(
            const SDOString& uri, 
            const SDOString& inTypeName,
            const SDOString& propname,
            const SDOString& subname,
            const SDOString& subTypeUri, 
            const SDOString& subTypeName)
    {
      setPropertySubstitute(uri.c_str(),
                            inTypeName.c_str(),
                            propname.c_str(),
                            subname.c_str(),
                            subTypeUri.c_str(),
                            subTypeName.c_str());
    }

    void DataFactoryImpl::setPropertySubstitute(
            const Type& containertype,
            const char* propname,
            const char* subname,
            const Type& subtype)
    {
        setPropertySubstitute(
            containertype.getURI(),
            containertype.getName(),
            propname,subname,
            subtype.getURI(),subtype.getName());
    }

    void DataFactoryImpl::setPropertySubstitute(
            const Type& containertype,
            const SDOString& propname,
            const SDOString& subname,
            const Type& subtype)
    {
      setPropertySubstitute(containertype, propname.c_str(), subname.c_str(), subtype);
    }
// ===================================================================
// setDefault - sets the default value for a property of a type
// ===================================================================

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname, bool b ) 
    {
        setDefault(t.getURI(), t.getName(), propname, b);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname, bool b ) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), b);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , char c) 
        
    {
        setDefault(t.getURI(), t.getName(), propname, c);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , char c) 
        
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), c);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , wchar_t c) 
    {
        setDefault(t.getURI(), t.getName(), propname, c);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , wchar_t c) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), c);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , char* c) 
    {
        setDefault(t.getURI(), t.getName(), propname, c);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , SDOString& outstr)
    {
        setDefault(t.getURI(), t.getName(), propname, outstr);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , short s) 
    {
        setDefault(t.getURI(), t.getName(), propname, s);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , short s) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), s);
    }

#if 32 !=64
    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , long l) 
    {
        setDefault(t.getURI(), t.getName(), propname, l);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , long l) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), l);
    }

#endif

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , int64_t i) 
    {
        setDefault(t.getURI(), t.getName(), propname, i);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , int64_t i) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), i);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , float f) 
    {
        setDefault(t.getURI(), t.getName(), propname, f);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , float f) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), f);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , long double d) 
    {
        setDefault(t.getURI(), t.getName(), propname, d);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , long double d) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), d);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , const SDODate d) 
    {
        setDefault(t.getURI(), t.getName(), propname, d);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , const SDODate d) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), d);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , const wchar_t* c, unsigned int len) 
    {
        setDefault(t.getURI(), t.getName(), propname, c, len);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , const wchar_t* c, unsigned int len) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), c, len);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const char* propname , const char* c, unsigned int len) 
    {
        setDefault(t.getURI(), t.getName(), propname, c, len);
    }

    void DataFactoryImpl::setDefault(
        const Type& t, const SDOString& propname , const SDOString& c, unsigned int len) 
    {
        setDefault(t.getURI(), t.getName(), propname.c_str(), c.c_str(), len);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname, bool b ) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(b);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname, bool b ) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), b);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , char c) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(c);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , char c) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), c);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , wchar_t c) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(c);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , wchar_t c) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), c);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , char* c) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(c);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , SDOString& c) 
    {
        const TypeImpl* ti = findTypeImpl(typuri, typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(c);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , short s) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(s);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , short s) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), s);
    }

#if 32 !=64
    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , long l) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(l);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , long l) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), l);
    }

#endif

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , int64_t i) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(i);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , int64_t i) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), i);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , float f) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(f);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , float f) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), f);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , long double d) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(d);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , long double d) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), d);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , const SDODate d) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(d);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , const SDODate d) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), d);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , const wchar_t* c, unsigned int len) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(c,len);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , const wchar_t* c, unsigned int len) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), c, len);
    }

    void DataFactoryImpl::setDefault(
        const char* typuri, const char* typnam, 
        const char* propname , const char* c, unsigned int len) 
    {
        const TypeImpl* ti = findTypeImpl(typuri,typnam);
        PropertyImpl* pi = ti->getPropertyImpl(propname);
        if (pi != 0)pi->setDefault(c,len);
    }

    void DataFactoryImpl::setDefault(
        const SDOString& typuri, const SDOString& typnam, 
        const SDOString& propname , const SDOString& c, unsigned int len) 
    {
      setDefault(typuri.c_str(), typnam.c_str(), propname.c_str(), c.c_str(), len);
    }

    void DataFactoryImpl::setOpposite(
        const Type& typ, 
        const char* propnam, 
        const Type& opptyp,
        const char* opppropnam) 
    {
        SDO_THROW_EXCEPTION("setOpposite" ,
        SDOUnsupportedOperationException, " Not implemented");
    }

    void DataFactoryImpl::setOpposite(
        const Type& typ, 
        const SDOString& propnam, 
        const Type& opptyp,
        const SDOString& opppropnam) 
    {
      setOpposite(typ, propnam.c_str(), opptyp, opppropnam.c_str());
    }

// ===================================================================
// getTypeImpl - return a pointer to the required TypeImpl
// ===================================================================
const TypeImpl& DataFactoryImpl::getTypeImpl(const SDOString& uri, const SDOString& inTypeName) const
{
    const TypeImpl* type = findTypeImpl(uri, inTypeName);

    if (type == 0)
    {
        string msg("Type not found :");
        msg += uri + " " + inTypeName;
        SDO_THROW_EXCEPTION("getTypeImpl" ,
        SDOTypeNotFoundException, msg.c_str());
    }
    
    return *type;;
}

// ===================================================================
// findType
// ===================================================================
const Type* DataFactoryImpl::findType(const SDOString uri, const SDOString inTypeName) const
{
    return (Type*) findTypeImpl(uri, inTypeName);
}

// ===================================================================
// findTypeImpl
// ===================================================================

const TypeImpl* DataFactoryImpl::findTypeImpl(const SDOString& uri, const SDOString& inTypeName) const
{
    SDOString fullTypeName;
	getFullTypeName(fullTypeName, uri, inTypeName);
    TYPES_MAP::const_iterator typeIter;
    typeIter = types.find(fullTypeName);
    if(typeIter != types.end())
    {
        return typeIter->second;
    }
    else
    {
        // try alias names
        getAliasTypeName(fullTypeName, uri, inTypeName);
        typeIter = types.find(fullTypeName);
        if(typeIter != types.end())
        {
            return typeIter->second;
        }
    }
    return 0;
}

// ===================================================================
// setAlias - sets a new alias for this type
// ===================================================================

void DataFactoryImpl::setAlias(const char* typeuri,
                              const char* typenam,
                              const char* alias)
{

    SDOString fullTypeName;
	getFullTypeName(fullTypeName, typeuri, typenam);
    TYPES_MAP::iterator typeIter;
    typeIter = types.find(fullTypeName);
    if(typeIter != types.end())
    {
        (typeIter->second)->setAlias(alias);
        getAliasTypeName(fullTypeName, typeuri, alias);
        types[fullTypeName] = typeIter->second;
    }

}

void DataFactoryImpl::setAlias(const SDOString& typeuri,
                              const SDOString& typenam,
                              const SDOString& alias)
{
    setAlias(typeuri.c_str(), typenam.c_str(), alias.c_str());
}

// ===================================================================
// setAlias - sets a new alias for this type
// ===================================================================

void DataFactoryImpl::setAlias(const char* typeuri, 
                              const char* typenam, 
                              const char* propname,
                              const char* alias)
{
    const TypeImpl&  t = getTypeImpl(typeuri, typenam);
    PropertyImpl* p  = t.getPropertyImpl(propname); 
    if (p != 0)p->setAlias(alias);

}

void DataFactoryImpl::setAlias(const SDOString& typeuri, 
                              const SDOString& typenam, 
                              const SDOString& propname,
                              const SDOString& alias)
{
    setAlias(typeuri.c_str(), typenam.c_str(), propname.c_str(), alias.c_str());
}

// ===================================================================
//  getTypes - gets the full list of types for this factory
// ===================================================================

TypeList DataFactoryImpl::getTypes() const
{
    TYPES_MAP::const_iterator typeIter;
    

    std::vector<const Type*> typeVector;

    for (typeIter = types.begin() ; typeIter != types.end();
    ++typeIter) {
        if (strncmp((typeIter->first).c_str(),"ALIAS::", 7)) {
            typeVector.insert(typeVector.end(),typeIter->second);
        }
    }


    return typeVector;
}


// ===================================================================
//  resolve - resolves the type and includes all the properties from
// the supertype. After this has been called, no further changes
// to the type hierarchy are allowed.
// ===================================================================

void DataFactoryImpl::resolve()
{

    TYPES_MAP::iterator typeIter;
    for (typeIter = resolvePending.begin() ; typeIter != resolvePending.end();
    ++typeIter) 
    {
        (typeIter->second)->initCompoundProperties();
        (typeIter->second)->validateChangeSummary();
    }
    // Need to empty the resolvePending set.
    resolvePending.clear();

}

// ===================================================================
//  create - creates a data object from the types available.
//  This first resolves the type hierarchy, and thus no further changes
//  to the type hierarchy are allowed.
// ===================================================================


RefCountingPointer<DataObject> DataFactoryImpl::create(const char* uri, const char* typeName) 
{

// New types can always be added now, so if there are any that haven't been
// resolved, do them now. The isResolved boolean is superseded by the
// resolvePending set being non-empty.

   if (!resolvePending.empty())
    {
        // Allow creation of types and properties before resolve.
        if (uri != 0 && !strcmp(uri,Type::SDOTypeNamespaceURI.c_str())) {
            if (!strcmp(typeName,"Type") || !strcmp(typeName,"Property"))
            {
                DataObject* dob = (DataObject*)(new DataObjectImpl(this, getTypeImpl(uri, typeName)));
                return dob;
            }
        }
        resolve();
    }

    const TypeImpl* ti = findTypeImpl(uri,typeName);
    if (ti == 0)
    {
        string msg("Instantiation of unknown type :");
        if (uri != 0) msg += uri;
        msg += " ";
        if (typeName != 0)msg += typeName;
        SDO_THROW_EXCEPTION("create" ,
        SDOTypeNotFoundException, msg.c_str());
    }

    if (ti->isAbstractType())
    {
        string msg("Instantiation of abstract type :");
        if (uri != 0)msg += uri;
        msg += " ";
        if (typeName != 0)msg += typeName;
        SDO_THROW_EXCEPTION("create" ,
        SDOUnsupportedOperationException, msg.c_str());
    }

    DataObject* dob = (DataObject*)(new DataObjectImpl(this, getTypeImpl(uri, typeName)));
    return dob;
}

// ===================================================================
//  create - creates a data object from the types available.
//  This first resolves the type hierarchy, and thus no further changes
//  to the type hierarchy are allowed.
// ===================================================================


RefCountingPointer<DataObject> DataFactoryImpl::create(const SDOString& uri, const SDOString& typeName) 
{
    return create(uri.c_str(), typeName.c_str());
}

// ===================================================================
// The openProperties map is a list of the curently present open
// properties as used by this factory. It will cause the 
// open properties to be written out as attributes and elements
// of the root data object when a graph is serialized.
// ===================================================================
    const propertyMap& DataFactoryImpl::getOpenProperties()
    {
        return openProperties;
    }

    void DataFactoryImpl::addOpenProperty(const PropertyImpl& prop)
    {
        propertyMap::iterator i;
        while ((i = openProperties.find(prop.getName())) !=
               openProperties.end())
        {
            openProperties.erase(i);
        }
        openProperties.insert(make_pair(prop.getName(),prop));
    }

    void DataFactoryImpl::removeOpenProperty(const SDOString& name)
    {
        propertyMap::iterator i = 
            openProperties.find(name.c_str());
        if (i != openProperties.end())
        {
            openProperties.erase(i);
        }
    }


// ===================================================================
//  create - creates a data object from the types available.
//  This first resolves the type hierarchy, and thus no further changes
//  to the type hierarchy are allowed.
// ===================================================================

RefCountingPointer<DataObject>    DataFactoryImpl::create(const Type& type) 
{
    return create( type.getURI(), type.getName());
}


// ===================================================================
//  setDASValue - Set a value on a Type
// ===================================================================
void DataFactoryImpl::setDASValue(const Type& type,
                                        const char* name, 
                                        DASValue* value)
{
    setDASValue(type.getURI(), type.getName(), name, value);
}

void DataFactoryImpl::setDASValue(const Type& type,
                                        const SDOString& name, 
                                        DASValue* value)
{
    setDASValue(type.getURI(), type.getName(), name.c_str(), value);
}

void DataFactoryImpl::setDASValue(const char* typeuri,
                                        const char* typenam, 
                                        const char* name, 
                                        DASValue* value)
{
    TypeImpl* type = (TypeImpl*)findTypeImpl(typeuri, typenam);
    if (type != NULL)
    {
        type->setDASValue(name, value);
    }
}

void DataFactoryImpl::setDASValue(const SDOString& typeuri,
                                        const SDOString& typenam, 
                                        const SDOString& name, 
                                        DASValue* value)
{
    setDASValue(typeuri.c_str(), typenam.c_str(), name.c_str(), value);
}

// ===================================================================
//  getDASValue - retrieve a value from a Type
// ===================================================================

DASValue* DataFactoryImpl::getDASValue(const Type& type,
                                                const char* name) const
{
    return getDASValue(type.getURI(), type.getName(), name);
}

DASValue* DataFactoryImpl::getDASValue(const Type& type,
                                       const SDOString& name) const
{
    return getDASValue(type.getURI(), type.getName(), name.c_str());
}

DASValue* DataFactoryImpl::getDASValue(const char* typeuri,
                                                const char* typenam,
                                                const char* name) const
{
    TypeImpl* type = (TypeImpl*)findTypeImpl(typeuri, typenam);
    if (type != NULL)
    {
        return type->getDASValue(name);
    }

    return NULL;    
}

DASValue* DataFactoryImpl::getDASValue(const SDOString& typeuri,
                                       const SDOString& typenam,
                                       const SDOString& name) const
{
    return getDASValue(typeuri.c_str(), typenam.c_str(), name.c_str());
}

// ===================================================================
//  setDASValue - Set a value on a Property
// ===================================================================
void DataFactoryImpl::setDASValue( 
                const Type& type,
                const char* propertyName,
                const char* name,
                DASValue* value)
{
    setDASValue(type.getURI(), type.getName(), propertyName, name, value);
}

void DataFactoryImpl::setDASValue( 
                const Type& type,
                const SDOString& propertyName,
                const SDOString& name,
                DASValue* value)
{
    setDASValue(type.getURI(), type.getName(), propertyName.c_str(), name.c_str(), value);
}

void DataFactoryImpl::setDASValue( 
                const char* typeuri,
                const char* typenam,
                const char* propertyName,
                const char* name,
                DASValue* value)
{
    const TypeImpl* type = findTypeImpl(typeuri, typenam);
    if (type != NULL)
    {
        PropertyImpl* prop = type->getPropertyImplPure(propertyName);
        if (prop != 0)prop->setDASValue(name, value);
    }
}

void DataFactoryImpl::setDASValue( 
                const SDOString& typeuri,
                const SDOString& typenam,
                const SDOString& propertyName,
                const SDOString& name,
                DASValue* value)
{
    setDASValue(typeuri.c_str(), typenam.c_str(), propertyName.c_str(), name.c_str(), value);
}

// ===================================================================
//  getDASValue - retrieve a value from a Property
// ===================================================================
DASValue* DataFactoryImpl::getDASValue( 
                const Type& type,
                const char* propertyName,
                const char* name) const
{
    return getDASValue(type.getURI(), type.getName(), propertyName, name);
}

DASValue* DataFactoryImpl::getDASValue( 
                const Type& type,
                const SDOString& propertyName,
                const SDOString& name) const
{
    return getDASValue(type.getURI(), type.getName(), propertyName.c_str(), name.c_str());
}

DASValue* DataFactoryImpl::getDASValue(
                const char* typeuri,
                const char* typenam,
                const char* propertyName, 
                const char* name) const
{
    const TypeImpl* type = findTypeImpl(typeuri, typenam);
    if (type != NULL)
    {
        try
        {
            PropertyImpl* prop = type->getPropertyImpl(propertyName);
            if (prop != 0)return prop->getDASValue(name);
        }
        catch (const SDOPropertyNotFoundException&)
        {
            // Ignore - return null
        }
    }

    return NULL;    
}

DASValue* DataFactoryImpl::getDASValue(
                const SDOString& typeuri,
                const SDOString& typenam,
                const SDOString& propertyName, 
                const SDOString& name) const
{
    return getDASValue(typeuri.c_str(), typenam.c_str(), propertyName.c_str(), name.c_str());
}


bool DataFactoryImpl::generateInterface(const char* fileroot, const char* factoryname)
{

    FILE* header;
    FILE* body;
    
    if ((fileroot == 0) || (strlen(fileroot) == 0)) return false;

    if (factoryname == 0 || (strlen(factoryname) == 0)) return false;
    

    char *headername = new char[strlen(fileroot) + 5];
    char *bodyname   = new char[strlen(fileroot) + 5];
    strcpy(headername,fileroot);
    strcpy(bodyname,fileroot);
    strcat(headername,".h");
    strcat(bodyname,".cpp");

    // here we should check the files - and if they are present with
    // contents we should read them, and merge the new generated code
    // I think the way to do that is to allow sections in between comment
    // pairs in three locations:
    // a) outside any method in the CPP file.
    // b) in the header file anywhere.
    // c) inside method definitions in cpp, either before, or after the
    // entire generated contents.
    //
    // 

    header = fopen(headername,"w+");
    if (header == 0) return false;

    body = fopen(bodyname,"w+");
    if (body == 0) return false;

 
    fprintf(header,"/*******************************************\n");
    fprintf(header," * Generated Typesafe Interface Header     *\n");
    fprintf(header," *******************************************/\n\n\n");
    fprintf(header,"#include \"commonj/sdo/SDO.h\"\n");
    fprintf(header,"using namespace commonj::sdo;\n\n\n");

    fprintf(body,"/*******************************************\n");
    fprintf(body," * Generated Typesafe Interface Body       *\n");
    fprintf(body," *******************************************/\n\n\n");
    fprintf(body,"#include \"%s\"\n",headername);
    fprintf(body,"using namespace commonj::sdo;\n\n\n");

    delete headername;
    delete bodyname;
   
    try {

        int nscount;

        TypeList tl = getTypes();

        // forward declarations and smart pointers

        fprintf(header,"/*******************************************\n");
        fprintf(header," * Forward declarations and smart pointers *\n");
        fprintf(header," *******************************************/\n\n\n");

        unsigned int i;
        for (i=0;i<tl.size();i++)
        {
            nscount = 0;

            if (!strcmp(tl[i].getURI(),"commonj.sdo")) continue;

            const char *uri = tl[i].getURI();
            char *c = (char*)strchr(uri,'.');

            if (c == 0)
            {
                fprintf(header,"namespace %s{\n",uri);
                nscount = 1;
            }
            else
            {
                char* buf = new char[strlen(uri) + 1];
                strcpy(buf, uri);
                c = buf;
                char* c1;
                do {
                    c1 = strchr(c,'.');
                    if (c1) *c1 = 0;
                    fprintf(header,"namespace %s{\n", c);
                    nscount++;
                    if (c1) c = c1+1;
                } while (c1 != 0);
                delete buf;
            }

            const char* name = tl[i].getName();

            fprintf(header,"    class %s;\n",name);
            fprintf(header,"    typedef RefCountingPointer<%s> %sPtr;\n",
                name, name);

            for (int i=0;i<nscount;i++)
            {
                fprintf(header,"}\n");
            }
        }
        fprintf(header,"\n\n");
        fprintf(header,"/*******************************************\n");
        fprintf(header," * The Data factory                        *\n");
        fprintf(header," *******************************************/\n\n\n");

        fprintf(header,"class %sDataFactory {\n", factoryname);
        fprintf(header,"    public:\n");
        fprintf(header,"    static %sDataFactory* get%sDataFactory();\n",
            factoryname, factoryname);

        for (i=0;i<tl.size();i++)
        {        

            if (!strcmp(tl[i].getURI(),"commonj.sdo")) continue;

            const char *uri = tl[i].getURI();
            const char *name = tl[i].getName();
            char* the_uri = (char*) strchr(uri,'.');

            if (the_uri == 0)
            {
                the_uri = new char[strlen(uri) + 1];
                strcpy(the_uri,uri);
            }
            else
            {
                the_uri = new char[2 * strlen(uri) + 1];
                int jj=0;
                for (unsigned int ii=0;ii<strlen(uri);ii++)
                {
                    if (uri[ii] == '.')
                    {
                        the_uri[jj++]=':';
                        the_uri[jj++]=':';
                    }
                    else
                    {
                        the_uri[jj++] = uri[ii];
                    }
                }
                the_uri[jj] = 0;
            }

            fprintf(header,"    %s::%sPtr create%s();\n", the_uri, name, name);

            fprintf(body,"%s::%sPtr %sDataFactory::create%s()\n",the_uri, name, factoryname, name);
            fprintf(body,"{\n");
            fprintf(body,"    DataObjectPtr dob = the_factory->create(\"%s\",\"%s\");\n",
                tl[i].getURI(),name);
            fprintf(body,"    %s::%s* the_ob = new %s::%s(dob);\n",the_uri,name,the_uri,name);
            fprintf(body,"    return the_ob;\n");
            fprintf(body,"}\n\n");

            delete the_uri;

        }

        fprintf(header,"    DataFactory* getDataFactory()\n");
        fprintf(header,"    {\n");
        fprintf(header,"        return (DataFactory*)the_factory;\n");
        fprintf(header,"    }\n");
        fprintf(header,"    private:\n");
        fprintf(header,"    DataFactoryPtr the_factory;\n");
        fprintf(header,"};\n");
    
        fprintf(header,"\n\n");

        fprintf(body,"%sDataFactory* %sDataFactory::get%sDataFactory()\n",
            factoryname,factoryname,factoryname);
        fprintf(body,"{\n");
        fprintf(body,"    %sDataFactory* t = new %sDataFactory();\n",
            factoryname, factoryname);
        fprintf(body,"    t->the_factory = DataFactory::getDataFactory();\n");
        fprintf(body,"    return t;\n");
        fprintf(body,"}\n\n");


        fprintf(header,"/*******************************************\n");
        fprintf(header," * DataObject Type definitions             *\n");
        fprintf(header," *******************************************/\n\n\n");


        for (i=0;i<tl.size();i++)
        {
            nscount = 0;

            if (!strcmp(tl[i].getURI(),"commonj.sdo")) continue;
            const char *uri = tl[i].getURI();
            char *c = (char*) strchr(uri,'.');

            if (c == 0)
            {
                fprintf(header,"namespace %s{\n",uri);
                fprintf(body,  "namespace %s{\n",uri);
                nscount = 1;
            }
            else
            {
                char* buf = new char[strlen(uri) + 1];
                strcpy(buf, uri);
                c = buf;
                char* c1;
                do {
                    c1 = strchr(c,'.');
                    if (c1) *c1 = 0;
                    fprintf(header,"namespace %s{\n", c);
                    fprintf(body,  "namespace %s{\n", c);
                    nscount++;
                    if (c1) c = c1+1;
                } while (c1 != 0);
                delete buf;
            }


            const char* name = tl[i].getName();

            fprintf(header,"    class %s :public RefCountingObject {\n", name);
            fprintf(header,"    public:\n");
            fprintf(header,"    %s(DataObject* d);",name);

            // construction from a data object

            fprintf(body,"%s::%s(DataObject* d)\n",name,name);
            fprintf(body,"{\n");
            fprintf(body,"    the_object = d;\n");
            fprintf(body,"    the_object->setUserData((void*)this);\n");
            fprintf(body,"}\n\n\n");



            PropertyList pl = tl[i].getProperties();
            for (unsigned int j=0;j<pl.size();j++)
            {
                const char* pname = pl[j].getName();

                if (pl[j].isMany())
                {
                    fprintf(header,"    DataObjectList& get%s();\n",pname);
                    fprintf(body,  "DataObjectList& %s::get%s\n{\n",name,pname);
                    fprintf(body,  "    return the_object->getList(\"%s\");\n",pname);
                    fprintf(body,  "}\n\n");
                }
                else 
                {
                    if (pl[j].getType().isDataType())
                    {
                        switch (pl[j].getTypeEnum())
                        {
                        case Type::BooleanType:
                            fprintf(header,"    bool get%s();\n",pname);
                            fprintf(body,  "bool %s::get%s\n(){\n",name, pname);
                            fprintf(body,  "    return the_object->getBoolean(\"%s\");\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(bool b);\n",pname);
                            fprintf(body,  "void %s::set%s(bool b)\n{\n",name, pname);
                            fprintf(body,  "    the_object->setBoolean(\"%s\",b);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");

                        break;
     
                        case Type::ByteType:
                         
                            fprintf(header,"    char get%s();\n",pname);
                            fprintf(body,  "char %s::get%s\n{\n",name, pname);
                            fprintf(body,  "    return the_object->getByte(\"%s\");\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(char c);\n",pname);
                            fprintf(body,  "void %s::set%s(char c)\n{\n",name, pname);
                            fprintf(body,  "    the_object->setByte(\"%s\",c);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");
     
                        break;
     
                        case Type::BytesType:
     
                            fprintf(header,"    unsigned int get%s(char *buf, unsigned int len);\n",pname);
                            fprintf(body,  "unsigned int %s::get%s(char *buf, unsigned int len)\n{\n",name,
                            pname);
                            fprintf(body,  "    return the_object->getBytes(\"%s\", buf,len);\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(char *buf,unsigned int len);\n",pname);
                            fprintf(body,  "void %s::set%s(char *buf, unsigned int len)\n{\n",name,
                            pname);
                            fprintf(body,  "    the_object->setBytes(\"%s\", buf,len);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");

                        break;
                         
                        case Type::CharacterType:
     
                            fprintf(header,"    wchar_t get%s();\n",pname);
                            fprintf(body,  "wchar_t %s::get%s\n{\n",name, pname);
                            fprintf(body,  "    return the_object->getCharacter(\"%s\");\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(wchar_t c);\n",pname);
                            fprintf(body,  "void %s::set%s(wchar_t c)\n{\n",name, pname);
                            fprintf(body,  "    the_object->setCharacter(\"%s\",c);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");
     
                        break;
     
                        case Type::DateType:
                         
                            fprintf(header,"    SDODate get%s();\n",pname);
                            fprintf(body,  "SDODate %s::get%s\n{\n",name, pname);
                            fprintf(body,  "    return the_object->getDate(\"%s\");\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(const SDODate c);\n",pname);
                            fprintf(body,  "void %s::set%s(const SDODate c)\n{\n",name, pname);
                            fprintf(body,  "    the_object->setDate(\"%s\",c);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");

                        break;
     
                        case Type::DoubleType:
                         
                            fprintf(header,"    long double get%s();\n",pname);
                            fprintf(body,  "long double %s::get%s\n{\n",name, pname);
                            fprintf(body,  "    return the_object->getDouble(\"%s\");\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(long double c);\n",pname);
                            fprintf(body,  "void %s::set%s(long double c)\n{\n",name, pname);
                            fprintf(body,  "    the_object->setDouble(\"%s\",c);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");
                                
                        break;
                         
                        case Type::FloatType:
     
                            fprintf(header,"    float get%s();\n",pname);
                            fprintf(body,  "float %s::get%s\n{\n",name, pname);
                            fprintf(body,  "    return the_object->getFloat(\"%s\");\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(float c);\n",pname);
                            fprintf(body,  "void %s::set%s(float c)\n{\n",name, pname);
                            fprintf(body,  "    the_object->setFloat(\"%s\",c);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");
                          
                        break;
     
                        case Type::IntegerType:
     
                            fprintf(header,"    long get%s();\n",pname);
                            fprintf(body,  "long %s::get%s\n{\n",name, pname);
                            fprintf(body,  "    return the_object->getInteger(\"%s\");\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(long c);\n",pname);
                            fprintf(body,  "void %s::set%s(long c)\n{\n",name, pname);
                            fprintf(body,  "    the_object->setInteger(\"%s\",c);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");
     
                        break;
                         
                        case Type::LongType:
     
                            fprintf(header,"    int64_t get%s();\n",pname);
                            fprintf(body,  "int64_t %s::get%s\n{\n",name, pname);
                            fprintf(body,  "    return the_object->getLong(\"%s\");\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(int64_t c);\n",pname);
                            fprintf(body,  "void %s::set%s(int64_t c)\n{\n",name, pname);
                            fprintf(body,  "    the_object->setLong(\"%s\",c);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");
     
                        break;
     
                        case Type::ShortType:
     
                            fprintf(header,"    short get%s();\n",pname);
                            fprintf(body,  "short %s::get%s\n{\n",name, pname);
                            fprintf(body,  "    return the_object->getShort(\"%s\");\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(short c);\n",pname);
                            fprintf(body,  "void %s::set%s(short c)\n{\n",name, pname);
                            fprintf(body,  "    the_object->setShort(\"%s\",c);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");
                         
                        break;
                          
                        case Type::StringType:
                        case Type::UriType:
     
                            fprintf(header,"    unsigned int get%s(wchar_t *buf, unsigned int len);\n",pname);
                            fprintf(body,  "unsigned int %s::get%s(wchar_t *buf, unsigned int len)\n{\n",name,
                            pname);
                            fprintf(body,  "    return the_object->getString(\"%s\", buf,len);\n",pname);
                            fprintf(body,  "}\n\n");
                            fprintf(header,"    void set%s(wchar_t *buf,unsigned int len);\n",pname);
                            fprintf(body,  "void %s::set%s(wchar_t *buf, unsigned int len)\n{\n",name,
                            pname);
                            fprintf(body,  "    the_object->setString(\"%s\", buf,len);\nreturn;\n",pname);
                            fprintf(body,  "}\n\n");
                            
                        break;
                    
                        default:
                           fprintf(header,"/* unknown primitive:%s */\n",pname);
                        break;
                        }
                    }
                    else

                    {
                        const char* ttname = pl[j].getType().getName();

                        fprintf(header,"    %sPtr get%s();\n",ttname, pname);

                        fprintf(body,  "%sPtr %s::get%s()\n{\n",
                                                        ttname,
                                                        name,
                                                        pname);
                        fprintf(body,  "DataObjectPtr dob = the_object->getDataObject(\"%s\");\n",pname);
                        fprintf(body, "DataObject* d = (DataObject*)dob;\n");
                        fprintf(body,"%s* value = (%s*)(d->getUserData());\n",ttname, ttname);
                        fprintf(body,  "    return (%s*)value;\n", ttname);
                        fprintf(body,  "}\n\n");
                        fprintf(header,"    void set%s(%sPtr dob);\n",pname, ttname);
                        fprintf(body,  "void %s::set%s(%sPtr dob)\n{\n",
                                name,pname,ttname);
                        fprintf(body,"%s* ptr = dob;\n", ttname);
                        fprintf(body,"DataObject* the_obj = ptr->getDataObject();\n");
                        fprintf(body,"the_object->setDataObject(\"%s\", the_obj);\nreturn;\n",
                            pname);
                        fprintf(body,  "}\n\n");

                    }
                }
            } // for

            // now print the contained data object

            fprintf(header,"    DataObject* getDataObject()\n");
            fprintf(header,"    {\n");
            fprintf(header,"        return (DataObject*)the_object;\n");
            fprintf(header,"    }\n");
            fprintf(header,"    private:\n");
            fprintf(header,"    DataObjectPtr the_object;\n");

            fprintf(header,"};\n");
            for (int i=0;i<nscount;i++)
            {
                fprintf(header,"}\n");
                fprintf(body,"}\n");
            }
        }
        fclose (header);
        fclose (body);
    }
    catch (SDORuntimeException e)
    {
        cout << "Exception in code Generation" << endl;
        cout << e << endl;
    }
    return true;
}

bool DataFactoryImpl::generateInterface(const SDOString& fileroot, const SDOString& factoryname)
{
    return generateInterface(fileroot.c_str(), factoryname.c_str());
}
    
    std::ostream& DataFactoryImpl::printSelf(std::ostream &os)
    {
        SDOUtils::printTypes(os, this);
        return os;
    }

};
};

