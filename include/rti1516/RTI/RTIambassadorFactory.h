/***********************************************************************
  IEEE 1516.1 High Level Architecture Interface Specification C++ API
  File: RTI/RTIambassadorFactory.h
***********************************************************************/

#ifndef RTI_RTIambassadorFactory_h
#define RTI_RTIambassadorFactory_h

namespace rti1516
{
  class RTIambassador;
}

#include <RTI/SpecificConfig.h>
#include <RTI/Exception.h>
#include <vector>
#include <string>
#include <memory>

namespace rti1516
{
  class RTI_EXPORT RTIambassadorFactory
  {
  public:
    RTIambassadorFactory();
    
    virtual
    ~RTIambassadorFactory();
    
    // 10.35
    std::unique_ptr< RTIambassador >
    createRTIambassador(std::vector< std::wstring > & args);
  };
}

#endif // RTI_RTIambassadorFactory_h
