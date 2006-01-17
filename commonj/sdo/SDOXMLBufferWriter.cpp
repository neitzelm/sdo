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

#include "commonj/sdo/SDOXMLBufferWriter.h"
#include "commonj/sdo/SDORuntimeException.h"

namespace commonj
{
    namespace sdo
    {
        
        
        
        
        SDOXMLBufferWriter::SDOXMLBufferWriter(DataFactoryPtr dataFactory)
            : SDOXMLWriter(dataFactory)
        {
            buf = xmlBufferCreate();
            if (buf == NULL)
            {
                SDO_THROW_EXCEPTION("SDOXMLBufferWriter", SDOXMLParserException, "xmlBufferCreate failed");
            }
            setWriter(xmlNewTextWriterMemory(buf, 0));                
        }
        
        SDOXMLBufferWriter::~SDOXMLBufferWriter()
        {    
            freeWriter();
            xmlBufferFree(buf);
        }
        
        SDOXMLString SDOXMLBufferWriter::getBuffer()
        {
            freeWriter();
            return buf->content;
        }            
    } // End - namespace sdo
} // End - namespace commonj
