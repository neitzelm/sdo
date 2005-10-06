/* 
+----------------------------------------------------------------------+
| (c) Copyright IBM Corporation 2005.                                  | 
| All Rights Reserved.                                                 |
+----------------------------------------------------------------------+ 
|                                                                      | 
| Licensed under the Apache License, Version 2.0 (the "License"); you  | 
| may not use this file except in compliance with the License. You may | 
| obtain a copy of the License at                                      | 
|  http://www.apache.org/licenses/LICENSE-2.0                          |
|                                                                      | 
| Unless required by applicable law or agreed to in writing, software  | 
| distributed under the License is distributed on an "AS IS" BASIS,    | 
| WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      | 
| implied. See the License for the specific language governing         | 
| permissions and limitations under the License.                       | 
+----------------------------------------------------------------------+ 
| Author: Pete Robbins                                                 | 
+----------------------------------------------------------------------+ 

*/
/* $Id$ */

#ifndef _HELPERPROVIDER_H_
#define _HELPERPROVIDER_H_

#include "XSDHelper.h"
#include "XMLHelper.h"
#include "DataFactory.h"
#include "export.h"

namespace commonj
{
	namespace sdo
	{
		
		
		class HelperProvider
		{
		public:
			
			SDO_API static XSDHelperPtr getXSDHelper(DataFactoryPtr dataFactory=0);
			SDO_API static XMLHelperPtr getXMLHelper(DataFactoryPtr dataFactory=0);

		};
		
	} // End - namespace sdo
} // End - namespace commonj

#endif // _HELPERPROVIDER_H_