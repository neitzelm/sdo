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

/* $Rev: 241789 $ $Date: 2007-08-25 00:50:26 +0930 (Sat, 25 Aug 2007) $ */

#include "commonj/sdo/DataFactory.h"
#include "commonj/sdo/DataFactoryImpl.h"

namespace commonj{
namespace sdo{

/**  
 *    DataFactory holds metadata and creates DataObjects.
 *
 *    The DataFactory holds properties and types loaded from XSD, or
 *    created using the DataFactory API.
 */

    DataFactory::~DataFactory()
    {
    }

    RefCountingPointer<DataFactory> DataFactory::getDataFactory()
    {
    DataFactory* dob = (DataFactory*)(new DataFactoryImpl());
    return RefCountingPointer<DataFactory> (dob);
    }

    RefCountingPointer<DataFactory> DataFactory::clone()
    {
    const DataFactoryImpl* df = (const DataFactoryImpl*)this;
    DataFactory* dob = (DataFactory*)(new DataFactoryImpl(*df));
    return RefCountingPointer<DataFactory> (dob);
    }

};
};
