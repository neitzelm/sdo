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

#ifndef _SDOXMLSTREAMWRITER_H_
#define _SDOXMLSTREAMWRITER_H_

#include "commonj/sdo/disable_warn.h"

#include "commonj/sdo/SDOXMLBufferWriter.h"
#include <iostream>


namespace commonj
{
    namespace sdo
    {
        
/** 
 * SDOXMLStreamWriter extends SDOXMLWriter and writes a stream.
 */
        class SDOXMLStreamWriter : public SDOXMLBufferWriter
        {
            
        public:
            
            SDOXMLStreamWriter(std::ostream& outXML, DataFactoryPtr dataFactory = NULL);                
            virtual ~SDOXMLStreamWriter();
            
            int write(XMLDocumentPtr doc);
        private:
            std::ostream& outXmlStream;
            
        };
    } // End - namespace sdo
} // End - namespace commonj


#endif //_SDOXMLSTREAMWRITER_H_