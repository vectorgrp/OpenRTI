/* -*-c++-*- OpenRTI - Copyright (C) 2009-2016 Mathias Froehlich
 *
 * This file is part of OpenRTI.
 *
 * OpenRTI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * OpenRTI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenRTI.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef OpenRTI_RTI1516TestLib_h
#define OpenRTI_RTI1516TestLib_h

#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include <RTI/FederateAmbassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/RTIambassador.h>
#include <RTI/LogicalTime.h>
#include <RTI/LogicalTimeInterval.h>
#include <RTI/LogicalTimeFactory.h>
#include <RTI/RangeBounds.h>

#include <TestLib.h>

namespace OpenRTI {

inline bool
operator==(const rti1516::VariableLengthData& lhs, const rti1516::VariableLengthData& rhs)
{
  unsigned long size = lhs.size();
  if (size != rhs.size())
    return false;
  return 0 == memcmp(lhs.data(), rhs.data(), size);
}

inline bool
operator!=(const rti1516::VariableLengthData& lhs, const rti1516::VariableLengthData& rhs)
{ return !operator==(lhs, rhs); }

inline rti1516::VariableLengthData
toVariableLengthData(const char* s)
{
    rti1516::VariableLengthData variableLengthData;
    if (s)
        variableLengthData.setData(s, (unsigned long)strlen(s));
    return variableLengthData;
}

inline rti1516::VariableLengthData
toVariableLengthData(const std::string& s)
{
    rti1516::VariableLengthData variableLengthData;
    variableLengthData.setData(s.data(), (unsigned long)s.size());
    return variableLengthData;
}

inline std::string
toString(const rti1516::VariableLengthData& variableLengthData)
{
    if (!variableLengthData.size())
        return std::string();
    return std::string((const char*)variableLengthData.data(), variableLengthData.size());
}

inline rti1516::VariableLengthData
toVariableLengthData(const std::wstring& s)
{
    rti1516::VariableLengthData variableLengthData;
    variableLengthData.setData(s.data(), (unsigned long)(sizeof(std::wstring::value_type)*s.size()));
    return variableLengthData;
}

inline std::wstring
toWString(const rti1516::VariableLengthData& variableLengthData)
{
    if (!variableLengthData.size())
        return std::wstring();
    return std::wstring((const wchar_t*)variableLengthData.data(), (unsigned long)(variableLengthData.size()/sizeof(std::wstring::value_type)));
}

inline rti1516::VariableLengthData
toVariableLengthData(const Clock& c)
{
    // May be at some time make this endian safe
    rti1516::VariableLengthData variableLengthData;
    variableLengthData.setData(&c, (unsigned long)sizeof(c));
    return variableLengthData;
}

inline Clock
toClock(const rti1516::VariableLengthData& variableLengthData)
{
    Clock c;
    // May be at some time make this endian safe
    if (variableLengthData.size() == sizeof(Clock))
        memcpy(&c, variableLengthData.data(), (unsigned long)sizeof(Clock));
    return c;
}

inline rti1516::VariableLengthData
toVariableLengthData(unsigned u)
{
    // May be at some time make this endian safe
    rti1516::VariableLengthData variableLengthData;
    variableLengthData.setData(&u, (unsigned long)sizeof(u));
    return variableLengthData;
}

inline unsigned
toUnsigned(const rti1516::VariableLengthData& variableLengthData)
{
    unsigned u = (unsigned)-1;
    // May be at some time make this endian safe
    if (variableLengthData.size() == sizeof(unsigned))
        memcpy(&u, variableLengthData.data(), sizeof(unsigned));
    return u;
}

class OPENRTI_LOCAL RTI1516TestAmbassador : public RTITest::Ambassador, public rti1516::FederateAmbassador {
public:
  RTI1516TestAmbassador(const RTITest::ConstructorArgs& constructorArgs) :
    RTITest::Ambassador(constructorArgs),
    _logicalTimeFactoryName(L"HLAinteger64Time"),
    _synchronized(0)
  { }
  virtual ~RTI1516TestAmbassador()
  { }

  const rti1516::FederateHandle& getFederateHandle() const
  { return _federateHandle; }

  const std::wstring& getLogicalTimeFactoryName() const
  { return _logicalTimeFactoryName; }
  void setLogicalTimeFactoryName(const std::wstring& logicalTimeFactoryName)
  { _logicalTimeFactoryName = logicalTimeFactoryName; }

  virtual bool execJoined(rti1516::RTIambassador& ambassador) = 0;

  bool waitForAllFederates(rti1516::RTIambassador& ambassador)
  {
    _synchronized = 0;

    // FIXME need a test for concurrent announces
    try {
      ambassador.registerFederationSynchronizationPoint(getFederateType(), rti1516::VariableLengthData());
    } catch (const rti1516::Exception& e) {
      std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
      return false;
    } catch (...) {
      std::wcout << L"Unknown Exception!" << std::endl;
      return false;
    }

    try {
      Clock timeout = Clock::now() + Clock::fromSeconds(10);
      while (!_federateSet.empty()) {
        if (ambassador.evokeCallback(10.0))
          continue;
        if (timeout < Clock::now()) {
          std::wcout << L"Timeout waiting for other federates to join!" << std::endl;
          return false;
        }
      }
    } catch (const rti1516::Exception& e) {
      std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
      return false;
    } catch (...) {
      std::wcout << L"Unknown Exception!" << std::endl;
      return false;
    }

    // Fill for the next time
    _federateSet.insert(getFederateList().begin(), getFederateList().end());

    try {
      for (std::vector<std::wstring>::const_iterator i = getFederateList().begin(); i != getFederateList().end(); ++i) {
        ambassador.synchronizationPointAchieved(*i);
      }
    } catch (const rti1516::Exception& e) {
      std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
      return false;
    } catch (...) {
      std::wcout << L"Unknown Exception!" << std::endl;
      return false;
    }

    try {
      Clock timeout = Clock::now() + Clock::fromSeconds(10);
      while (_synchronized < getFederateList().size()) {
        if (ambassador.evokeCallback(10.0))
          continue;
        if (timeout < Clock::now()) {
          std::wcout << L"Timeout waiting for other federates to synchronize!" << std::endl;
          return false;
        }
      }
    } catch (const rti1516::Exception& e) {
      std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
      return false;
    } catch (...) {
      std::wcout << L"Unknown Exception!" << std::endl;
      return false;
    }

    return true;
  }

  virtual bool execJoinOnce()
  {
    std::unique_ptr<rti1516::RTIambassador> ambassador;
    rti1516::RTIambassadorFactory factory;
    std::vector<std::wstring> args = getArgumentList();
    ambassador = factory.createRTIambassador(args);

    // create, must work once
    try {
      ambassador->createFederationExecution(getFederationExecution(), getFddFile(), _logicalTimeFactoryName);

      if (!getFederationBarrier()->success())
        return false;
    } catch (const rti1516::FederationExecutionAlreadyExists&) {
      // Can happen in this test

      if (!getFederationBarrier()->fail())
        return false;
    } catch (const rti1516::Exception& e) {
      std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
      return false;
    } catch (...) {
      std::wcout << L"Unknown Exception!" << std::endl;
      return false;
    }

    // Try that several times. Ensure correct cleanup
    unsigned n = 99;
    for (unsigned i = 0; i < n; ++i) {

      // join must work
      try {
        _federateHandle = ambassador->joinFederationExecution(getFederateType(), getFederationExecution(), *this);
      } catch (const rti1516::Exception& e) {
        std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
        return false;
      } catch (...) {
        std::wcout << L"Unknown Exception!" << std::endl;
        return false;
      }

      _federateSet.insert(getFederateList().begin(), getFederateList().end());

      if (!execJoined(*ambassador))
        return false;

      // and now resign must work
      try {
        ambassador->resignFederationExecution(rti1516::CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (const rti1516::Exception& e) {
        std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
        return false;
      } catch (...) {
        std::wcout << L"Unknown Exception!" << std::endl;
        return false;
      }

    }

    // Wait for all threads, this is to ensure that we do not destroy before we are ready
    wait();

    // destroy, must work once
    try {
      ambassador->destroyFederationExecution(getFederationExecution());

      if (!getFederationBarrier()->success())
        return false;
    } catch (const rti1516::FederatesCurrentlyJoined&) {
      // Can happen in this test
      // Other threads just might have still joined.

      if (!getFederationBarrier()->fail())
        return false;
    } catch (const rti1516::FederationExecutionDoesNotExist&) {
      // Can happen in this test
      // Other threads might have been faster

      if (!getFederationBarrier()->fail())
        return false;
    } catch (const rti1516::Exception& e) {
      std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
      return false;
    } catch (...) {
      std::wcout << L"Unknown Exception!" << std::endl;
      return false;
    }

    return true;
  }

  virtual bool execJoinMultiple()
  {
    std::unique_ptr<rti1516::RTIambassador> ambassador;
    rti1516::RTIambassadorFactory factory;
    std::vector<std::wstring> args = getArgumentList();
    ambassador = factory.createRTIambassador(args);

    // Try that several times. Ensure correct cleanup
    unsigned n = 99;
    for (unsigned i = 0; i < n; ++i) {

      // create, must work once
      try {
        ambassador->createFederationExecution(getFederationExecution(), getFddFile(), _logicalTimeFactoryName);

        if (!getFederationBarrier()->success())
          return false;
      } catch (const rti1516::FederationExecutionAlreadyExists&) {
        // Can happen in this test

        if (!getFederationBarrier()->fail())
          return false;
      } catch (const rti1516::Exception& e) {
        std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
        return false;
      } catch (...) {
        std::wcout << L"Unknown Exception!" << std::endl;
        return false;
      }

      // join must work
      try {
        _federateHandle = ambassador->joinFederationExecution(getFederateType(), getFederationExecution(), *this);
      } catch (const rti1516::Exception& e) {
        std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
        return false;
      } catch (...) {
        std::wcout << L"Unknown Exception!" << std::endl;
        return false;
      }

      _federateSet.insert(getFederateList().begin(), getFederateList().end());

      if (!execJoined(*ambassador))
        return false;

      // and now resign must work
      try {
        ambassador->resignFederationExecution(rti1516::CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (const rti1516::Exception& e) {
        std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
        return false;
      } catch (...) {
        std::wcout << L"Unknown Exception!" << std::endl;
        return false;
      }

      // Wait for all threads, this is to ensure that we do not destroy before we are ready
      wait();

      // destroy, must work once
      try {
        ambassador->destroyFederationExecution(getFederationExecution());

        if (!getFederationBarrier()->success())
          return false;
      } catch (const rti1516::FederatesCurrentlyJoined&) {
        // Can happen in this test
        // Other threads just might have still joined.

        if (!getFederationBarrier()->fail())
          return false;
      } catch (const rti1516::FederationExecutionDoesNotExist&) {
        // Can happen in this test
        // Other threads might have been faster

        if (!getFederationBarrier()->fail())
          return false;
      } catch (const rti1516::Exception& e) {
        std::wcout << L"rti1516::Exception: \"" << e.what() << L"\"" << std::endl;
        return false;
      } catch (...) {
        std::wcout << L"Unknown Exception!" << std::endl;
        return false;
      }
    }

    return true;
  }

  virtual bool exec()
  {
    if (_constructorArgs._joinOnce)
      return execJoinOnce();
    else
      return execJoinMultiple();
  }

  virtual void synchronizationPointRegistrationSucceeded(const std::wstring& label)
  {
  }

  virtual void synchronizationPointRegistrationFailed(const std::wstring& label, rti1516::SynchronizationFailureReason reason)
  {
  }

  virtual void announceSynchronizationPoint(const std::wstring& label, const rti1516::VariableLengthData& tag)
  {
    _federateSet.erase(label);
  }

  virtual void federationSynchronized(const std::wstring& label)
  {
    ++_synchronized;
  }

  virtual void initiateFederateSave(const std::wstring& label)
  {
  }

  virtual void initiateFederateSave(const std::wstring& label, const rti1516::LogicalTime& logicalTime)
  {
  }

  virtual void federationSaved()
  {
  }

  virtual void federationNotSaved(rti1516::SaveFailureReason theSaveFailureReason)
  {
  }

  virtual void federationSaveStatusResponse(const rti1516::FederateHandleSaveStatusPairVector& theFederateStatusVector)
  {
  }

  virtual void requestFederationRestoreSucceeded(const std::wstring& label)
  {
  }

  virtual void requestFederationRestoreFailed(const std::wstring& label)
  {
  }

  virtual void federationRestoreBegun()
  {
  }

  virtual void initiateFederateRestore(const std::wstring& label, rti1516::FederateHandle handle)
  {
  }

  virtual void federationRestored()
  {
  }

  virtual void federationNotRestored(rti1516::RestoreFailureReason theRestoreFailureReason)
  {
  }

  virtual void federationRestoreStatusResponse(const rti1516::FederateHandleRestoreStatusPairVector& theFederateStatusVector)
  {
  }

  virtual void startRegistrationForObjectClass(rti1516::ObjectClassHandle)
  {
  }

  virtual void stopRegistrationForObjectClass(rti1516::ObjectClassHandle)
  {
  }

  virtual void turnInteractionsOn(rti1516::InteractionClassHandle)
  {
  }

  virtual void turnInteractionsOff(rti1516::InteractionClassHandle)
  {
  }

  virtual void objectInstanceNameReservationSucceeded(const std::wstring&)
  {
  }

  virtual void objectInstanceNameReservationFailed(const std::wstring&)
  {
  }

  virtual void discoverObjectInstance(rti1516::ObjectInstanceHandle, rti1516::ObjectClassHandle, const std::wstring&)
  {
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle, const rti1516::AttributeHandleValueMap&,
                                      const rti1516::VariableLengthData&, rti1516::OrderType, rti1516::TransportationType)
  {
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle, const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData& tag, rti1516::OrderType orderType, rti1516::TransportationType transportationType,
                                      const rti1516::RegionHandleSet&)
  {
    reflectAttributeValues(objectInstanceHandle, attributeHandleValueMap, tag, orderType, transportationType);
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle, const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData& tag, rti1516::OrderType orderType, rti1516::TransportationType transportationType,
                                      const rti1516::LogicalTime&, rti1516::OrderType)
  {
    reflectAttributeValues(objectInstanceHandle, attributeHandleValueMap, tag, orderType, transportationType);
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle, const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData& tag, rti1516::OrderType orderType, rti1516::TransportationType transportationType,
                                      const rti1516::LogicalTime& logicalTime, rti1516::OrderType receivedOrder, const rti1516::RegionHandleSet&)
  {
    reflectAttributeValues(objectInstanceHandle, attributeHandleValueMap, tag, orderType, transportationType, logicalTime, receivedOrder);
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle, const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData& tag, rti1516::OrderType orderType, rti1516::TransportationType transportationType,
                                      const rti1516::LogicalTime& logicalTime, rti1516::OrderType receivedOrder, rti1516::MessageRetractionHandle)
  {
    reflectAttributeValues(objectInstanceHandle, attributeHandleValueMap, tag, orderType, transportationType, logicalTime, receivedOrder);
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle, const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData& tag, rti1516::OrderType orderType, rti1516::TransportationType transportationType,
                                      const rti1516::LogicalTime& logicalTime, rti1516::OrderType receivedOrder, rti1516::MessageRetractionHandle,
                                      const rti1516::RegionHandleSet& regionHandleSet)
  {
    reflectAttributeValues(objectInstanceHandle, attributeHandleValueMap, tag, orderType, transportationType, logicalTime, receivedOrder, regionHandleSet);
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle, const rti1516::ParameterHandleValueMap&,
                                  const rti1516::VariableLengthData&, rti1516::OrderType, rti1516::TransportationType)
  {
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle theInteraction, const rti1516::ParameterHandleValueMap& theParameterValues,
                                  const rti1516::VariableLengthData& theUserSuppliedTag, rti1516::OrderType sentOrder, rti1516::TransportationType theType,
                                  const rti1516::RegionHandleSet&)
  {
    receiveInteraction(theInteraction, theParameterValues, theUserSuppliedTag, sentOrder, theType);
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle theInteraction,
                                  rti1516::ParameterHandleValueMap const & theParameterValues,
                                  rti1516::VariableLengthData const & theUserSuppliedTag,
                                  rti1516::OrderType sentOrder,
                                  rti1516::TransportationType theType,
                                  rti1516::LogicalTime const & theTime,
                                  rti1516::OrderType receivedOrder)
  {
    receiveInteraction(theInteraction, theParameterValues, theUserSuppliedTag, sentOrder, theType);
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle theInteraction,
                                  rti1516::ParameterHandleValueMap const & theParameterValues,
                                  rti1516::VariableLengthData const & theUserSuppliedTag,
                                  rti1516::OrderType sentOrder,
                                  rti1516::TransportationType theType,
                                  rti1516::LogicalTime const & theTime,
                                  rti1516::OrderType receivedOrder,
                                  rti1516::RegionHandleSet const & theSentRegionHandleSet)
  {
    receiveInteraction(theInteraction, theParameterValues, theUserSuppliedTag, sentOrder, theType, theTime, receivedOrder);
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle theInteraction,
                                  rti1516::ParameterHandleValueMap const & theParameterValues,
                                  rti1516::VariableLengthData const & theUserSuppliedTag,
                                  rti1516::OrderType sentOrder,
                                  rti1516::TransportationType theType,
                                  rti1516::LogicalTime const & theTime,
                                  rti1516::OrderType receivedOrder,
                                  rti1516::MessageRetractionHandle theHandle)
  {
    receiveInteraction(theInteraction, theParameterValues, theUserSuppliedTag, sentOrder, theType, theTime, receivedOrder);
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle theInteraction,
                                  rti1516::ParameterHandleValueMap const & theParameterValues,
                                  rti1516::VariableLengthData const & theUserSuppliedTag,
                                  rti1516::OrderType sentOrder,
                                  rti1516::TransportationType theType,
                                  rti1516::LogicalTime const & theTime,
                                  rti1516::OrderType receivedOrder,
                                  rti1516::MessageRetractionHandle theHandle,
                                  rti1516::RegionHandleSet const & theSentRegionHandleSet)
  {
    receiveInteraction(theInteraction, theParameterValues, theUserSuppliedTag, sentOrder, theType, theTime, receivedOrder, theSentRegionHandleSet);
  }

  virtual void removeObjectInstance(rti1516::ObjectInstanceHandle theObject,
                                    rti1516::VariableLengthData const & theUserSuppliedTag,
                                    rti1516::OrderType sentOrder)
  {
  }

  virtual void removeObjectInstance(rti1516::ObjectInstanceHandle theObject,
                                    rti1516::VariableLengthData const & theUserSuppliedTag,
                                    rti1516::OrderType sentOrder,
                                    rti1516::LogicalTime const & theTime,
                                    rti1516::OrderType receivedOrder)
  {
    removeObjectInstance(theObject, theUserSuppliedTag, sentOrder);
  }

  virtual void removeObjectInstance(rti1516::ObjectInstanceHandle theObject,
                                    rti1516::VariableLengthData const & theUserSuppliedTag,
                                    rti1516::OrderType sentOrder,
                                    rti1516::LogicalTime const & theTime,
                                    rti1516::OrderType receivedOrder,
                                    rti1516::MessageRetractionHandle theHandle)
  {
    removeObjectInstance(theObject, theUserSuppliedTag, sentOrder, theTime, receivedOrder);
  }

  virtual void attributesInScope(rti1516::ObjectInstanceHandle theObject,
                                 rti1516::AttributeHandleSet const & theAttributes)
  {
  }

  virtual void attributesOutOfScope(rti1516::ObjectInstanceHandle theObject,
                                    rti1516::AttributeHandleSet const & theAttributes)
  {
  }

  virtual void provideAttributeValueUpdate(rti1516::ObjectInstanceHandle theObject,
                                           rti1516::AttributeHandleSet const & theAttributes,
                                           rti1516::VariableLengthData const & theUserSuppliedTag)
  {
  }

  virtual void turnUpdatesOnForObjectInstance(rti1516::ObjectInstanceHandle theObject,
                                              rti1516::AttributeHandleSet const & theAttributes)
  {
  }

  virtual void turnUpdatesOffForObjectInstance(rti1516::ObjectInstanceHandle theObject,
                                               rti1516::AttributeHandleSet const & theAttributes)
  {
  }

  virtual void requestAttributeOwnershipAssumption(rti1516::ObjectInstanceHandle theObject,
                                                   rti1516::AttributeHandleSet const & offeredAttributes,
                                                   rti1516::VariableLengthData const & theUserSuppliedTag)
  {
  }

  virtual void requestDivestitureConfirmation(rti1516::ObjectInstanceHandle theObject,
                                              rti1516::AttributeHandleSet const & releasedAttributes)
  {
  }

  virtual void attributeOwnershipAcquisitionNotification(rti1516::ObjectInstanceHandle theObject,
                                                         rti1516::AttributeHandleSet const & securedAttributes,
                                                         rti1516::VariableLengthData const & theUserSuppliedTag)
  {
  }

  virtual void attributeOwnershipUnavailable(rti1516::ObjectInstanceHandle theObject,
                                             rti1516::AttributeHandleSet const & theAttributes)
  {
  }

  virtual void requestAttributeOwnershipRelease(rti1516::ObjectInstanceHandle theObject,
                                                rti1516::AttributeHandleSet const & candidateAttributes,
                                                rti1516::VariableLengthData const & theUserSuppliedTag)
  {
  }

  virtual void confirmAttributeOwnershipAcquisitionCancellation(rti1516::ObjectInstanceHandle theObject,
                                                                rti1516::AttributeHandleSet const & theAttributes)
  {
  }

  virtual void informAttributeOwnership(rti1516::ObjectInstanceHandle theObject,
                                        rti1516::AttributeHandle theAttribute,
                                        rti1516::FederateHandle theOwner)
  {
  }

  virtual void attributeIsNotOwned(rti1516::ObjectInstanceHandle theObject,
                                   rti1516::AttributeHandle theAttribute)
  {
  }

  virtual void attributeIsOwnedByRTI(rti1516::ObjectInstanceHandle theObject,
                                     rti1516::AttributeHandle theAttribute)
  {
  }

  virtual void timeRegulationEnabled(rti1516::LogicalTime const & theFederateTime)
  {
  }

  virtual void timeConstrainedEnabled(rti1516::LogicalTime const & theFederateTime)
  {
  }

  virtual void timeAdvanceGrant(rti1516::LogicalTime const & theTime)
  {
  }

  virtual void requestRetraction(rti1516::MessageRetractionHandle theHandle)
  {
  }

private:
  std::wstring _logicalTimeFactoryName;
  rti1516::FederateHandle _federateHandle;
  unsigned _synchronized;
  std::set<std::wstring> _federateSet;
};

class OPENRTI_LOCAL RTI1516SimpleAmbassador : public rti1516::FederateAmbassador {
public:
  RTI1516SimpleAmbassador() :
    // _fail(false),
    _useDataUrlObjectModels(false),
    _timeRegulationEnabled(false),
    _timeConstrainedEnabled(false),
    _timeAdvancePending(false)
  { }
  virtual ~RTI1516SimpleAmbassador()
  { }

  // bool getFail() const
  // { return _fail; }

  void setUseDataUrlObjectModels(bool useDataUrlObjectModels)
  { _useDataUrlObjectModels = useDataUrlObjectModels; }
  bool getUseDataUrlObjectModels() const
  { return _useDataUrlObjectModels; }

  bool getTimeRegulationEnabled() const
  { return _timeRegulationEnabled; }
  bool getTimeConstrainedEnabled() const
  { return _timeConstrainedEnabled; }
  bool getTimeAdvancePending() const
  { return _timeAdvancePending; }

  const rti1516::FederateHandle& getFederateHandle() const
  { return _federateHandle; }

  void connect(std::vector<std::wstring> args)
  {
    rti1516::RTIambassadorFactory factory;
    _ambassador = factory.createRTIambassador(args);
    setLogicalTimeFactory();
  }

  void setLogicalTimeFactory(const std::wstring& logicalTimeImplementationName = std::wstring(L"HLAinteger64Time"))
  {
    _logicalTimeImplementationName = logicalTimeImplementationName;
    _logicalTimeFactory = rti1516::LogicalTimeFactoryFactory::makeLogicalTimeFactory(logicalTimeImplementationName);
  }

  void createFederationExecution(const std::wstring& federationExecutionName, std::wstring fddFile)
  {
    _replaceFileWithDataIfNeeded(fddFile);
    _ambassador->createFederationExecution(federationExecutionName, fddFile, _logicalTimeImplementationName);
  }

  void destroyFederationExecution(const std::wstring& federationExecutionName)
  {
    _ambassador->destroyFederationExecution(federationExecutionName);
  }

  const rti1516::FederateHandle& joinFederationExecution(const std::wstring& federateType,
                                                         const std::wstring& federationExecutionName)
  {
    _federateHandle = _ambassador->joinFederationExecution(federateType, federationExecutionName, *this);
    _grantedLogicalTime = _logicalTimeFactory->makeLogicalTime();
    _grantedLogicalTime->setInitial();
    return _federateHandle;
  }

  void resignFederationExecution(rti1516::ResignAction resignAction)
  {
    _ambassador->resignFederationExecution(resignAction);
    _federateHandle = rti1516::FederateHandle();
  }

  void registerFederationSynchronizationPoint(const std::wstring& label, const rti1516::VariableLengthData& tag)
  {
    _ambassador->registerFederationSynchronizationPoint(label, tag);
  }

  void registerFederationSynchronizationPoint(const std::wstring& label, const rti1516::VariableLengthData& tag,
                                              const rti1516::FederateHandleSet& federateHandleSet)
  {
    _ambassador->registerFederationSynchronizationPoint(label, tag, federateHandleSet);
  }

  void synchronizationPointAchieved(const std::wstring& label)
  {
    _ambassador->synchronizationPointAchieved(label);
  }

  void requestFederationSave(const std::wstring& label)
  {
    _ambassador->requestFederationSave(label);
  }

  void requestFederationSave(const std::wstring& label, const rti1516::LogicalTime& logicalTime)
  {
    _ambassador->requestFederationSave(label, logicalTime);
  }

  void federateSaveComplete()
  {
    _ambassador->federateSaveComplete();
  }

  void federateSaveNotComplete()
  {
    _ambassador->federateSaveNotComplete();
  }

  void queryFederationSaveStatus()
  {
    _ambassador->queryFederationSaveStatus();
  }

  void requestFederationRestore(const std::wstring& label)
  {
    _ambassador->requestFederationRestore(label);
  }

  void federateRestoreComplete()
  {
    _ambassador->federateRestoreComplete();
  }

  void federateRestoreNotComplete()
  {
    _ambassador->federateRestoreNotComplete();
  }

  void queryFederationRestoreStatus()
  {
    _ambassador->queryFederationRestoreStatus();
  }

  void publishObjectClassAttributes(const rti1516::ObjectClassHandle& objectClassHandle,
                                    const rti1516::AttributeHandleSet& attributeList)
  {
    _ambassador->publishObjectClassAttributes(objectClassHandle, attributeList);
  }

  void unpublishObjectClass(const rti1516::ObjectClassHandle& objectClassHandle)
  {
    _ambassador->unpublishObjectClass(objectClassHandle);
  }

  void unpublishObjectClassAttributes(const rti1516::ObjectClassHandle& objectClassHandle,
                                      const rti1516::AttributeHandleSet& attributeList)
  {
    _ambassador->unpublishObjectClassAttributes(objectClassHandle, attributeList);
  }

  void publishInteractionClass(const rti1516::InteractionClassHandle& interactionClassHandle)
  {
    _ambassador->publishInteractionClass(interactionClassHandle);
  }

  void unpublishInteractionClass(const rti1516::InteractionClassHandle& interactionClassHandle)
  {
    _ambassador->unpublishInteractionClass(interactionClassHandle);
  }

  void subscribeObjectClassAttributes(const rti1516::ObjectClassHandle& objectClassHandle,
                                      const rti1516::AttributeHandleSet& attributeHandleSet,
                                      bool active = true)
  {
      _ambassador->subscribeObjectClassAttributes(objectClassHandle, attributeHandleSet, active);
      // _subscribedObjectClassAttributeHandleSetMap[objectClassHandle].insert(attributeHandleSet.begin(), attributeHandleSet.end());
  }

  void unsubscribeObjectClass(const rti1516::ObjectClassHandle& objectClassHandle)
  {
      _ambassador->unsubscribeObjectClass(objectClassHandle);
      // _subscribedObjectClassAttributeHandleSetMap.erase(objectClassHandle);
  }

  void unsubscribeObjectClassAttributes(const rti1516::ObjectClassHandle& objectClassHandle,
                                        const rti1516::AttributeHandleSet& attributeHandleSet)
  {
      _ambassador->unsubscribeObjectClassAttributes(objectClassHandle, attributeHandleSet);
      // ObjectClassAttributeHandleSetMap::iterator i = _subscribedObjectClassAttributeHandleSetMap.find(objectClassHandle);
      // for (rti1516::AttributeHandleSet::const_iterator j = attributeHandleSet.begin();
      //      j != attributeHandleSet.end(); ++j)
      //     i->second.erase(*j);
  }

  void subscribeInteractionClass(const rti1516::InteractionClassHandle& interactionClassHandle, bool active = true)
  {
      _ambassador->subscribeInteractionClass(interactionClassHandle, active);
  }

  void unsubscribeInteractionClass(const rti1516::InteractionClassHandle& interactionClassHandle)
  {
      _ambassador->unsubscribeInteractionClass(interactionClassHandle);
  }

  void reserveObjectInstanceName(const std::wstring& objectInstanceName)
  {
      _ambassador->reserveObjectInstanceName(objectInstanceName);
  }

  rti1516::ObjectInstanceHandle registerObjectInstance(const rti1516::ObjectClassHandle& objectClassHandle)
  {
      return _ambassador->registerObjectInstance(objectClassHandle);
  }

  rti1516::ObjectInstanceHandle registerObjectInstance(const rti1516::ObjectClassHandle& objectClassHandle,
                                                       const std::wstring& objectInstanceName)
  {
      return _ambassador->registerObjectInstance(objectClassHandle, objectInstanceName);
  }

  void updateAttributeValues(const rti1516::ObjectInstanceHandle& objectInstanceHandle,
                             const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                             const rti1516::VariableLengthData& tag)
  {
      _ambassador->updateAttributeValues(objectInstanceHandle, attributeHandleValueMap, tag);
  }

  rti1516::MessageRetractionHandle updateAttributeValues(const rti1516::ObjectInstanceHandle& objectInstanceHandle,
                                                         const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                                         const rti1516::VariableLengthData& tag,
                                                         const rti1516::LogicalTime& logicalTime)
  {
    return _ambassador->updateAttributeValues(objectInstanceHandle, attributeHandleValueMap, tag, logicalTime);
  }

  void sendInteraction(const rti1516::InteractionClassHandle& interactionClassHandle,
                       const rti1516::ParameterHandleValueMap& parameterHandleValueMap,
                       const rti1516::VariableLengthData& tag)
  {
      _ambassador->sendInteraction(interactionClassHandle, parameterHandleValueMap, tag);
  }

  rti1516::MessageRetractionHandle sendInteraction(const rti1516::InteractionClassHandle& interactionClassHandle,
                                                   const rti1516::ParameterHandleValueMap& parameterHandleValueMap,
                                                   const rti1516::VariableLengthData& tag,
                                                   const rti1516::LogicalTime& logicalTime)
  {
      return _ambassador->sendInteraction(interactionClassHandle, parameterHandleValueMap, tag, logicalTime);
  }

  void deleteObjectInstance(const rti1516::ObjectInstanceHandle& objectInstanceHandle,
                            const rti1516::VariableLengthData& tag)
  {
    _ambassador->deleteObjectInstance(objectInstanceHandle, tag);
  }

  rti1516::MessageRetractionHandle deleteObjectInstance(const rti1516::ObjectInstanceHandle& objectInstanceHandle,
                                                        const rti1516::VariableLengthData& tag,
                                                        const rti1516::LogicalTime& logicalTime)
  {
    return _ambassador->deleteObjectInstance(objectInstanceHandle, tag, logicalTime);
  }

  void localDeleteObjectInstance(const rti1516::ObjectInstanceHandle& objectInstanceHandle)
  {
    _ambassador->localDeleteObjectInstance(objectInstanceHandle);
  }

  void changeAttributeTransportationType(const rti1516::ObjectInstanceHandle& objectInstanceHandle,
                                         const rti1516::AttributeHandleSet& attributeHandleSet,
                                         const rti1516::TransportationType& transportationType)
  {
    _ambassador->changeAttributeTransportationType(objectInstanceHandle, attributeHandleSet, transportationType);
  }

  void changeInteractionTransportationType(const rti1516::InteractionClassHandle& interactionClassHandle,
                                           const rti1516::TransportationType& transportationType)
  {
    _ambassador->changeInteractionTransportationType(interactionClassHandle, transportationType);
  }

  void requestAttributeValueUpdate(const rti1516::ObjectInstanceHandle& objectInstanceHandle,
                                   const rti1516::AttributeHandleSet& attributeHandleSet,
                                   const rti1516::VariableLengthData& tag)
  {
    _ambassador->requestAttributeValueUpdate(objectInstanceHandle, attributeHandleSet, tag);
  }

  void requestAttributeValueUpdate(const rti1516::ObjectClassHandle& objectClassHandle,
                                   const rti1516::AttributeHandleSet& attributeHandleSet,
                                   const rti1516::VariableLengthData& tag)
  {
    _ambassador->requestAttributeValueUpdate(objectClassHandle, attributeHandleSet, tag);
  }

  void unconditionalAttributeOwnershipDivestiture(const rti1516::ObjectInstanceHandle& objectInstanceHandle,
                                                  const rti1516::AttributeHandleSet& attributeHandleSet)
  {
    _ambassador->unconditionalAttributeOwnershipDivestiture(objectInstanceHandle, attributeHandleSet);
  }

    // // 7.3
    //  void negotiatedAttributeOwnershipDivestiture
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSet const & attributes,
    //  VariableLengthData const & tag)

    // // 7.6
    //  void confirmDivestiture
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSet const & confirmedAttributes,
    //  VariableLengthData const & tag)

    // // 7.8
    //  void attributeOwnershipAcquisition
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSet const & desiredAttributes,
    //  VariableLengthData const & tag)

    // // 7.9
    //  void attributeOwnershipAcquisitionIfAvailable
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSet const & desiredAttributes)

    // // 7.12
    //  void attributeOwnershipDivestitureIfWanted
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSet const & attributes,
    //  AttributeHandleSet & theDivestedAttributes) // filled by RTI

    // // 7.13
    //  void cancelNegotiatedAttributeOwnershipDivestiture
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSet const & attributes)

    // // 7.14
    //  void cancelAttributeOwnershipAcquisition
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSet const & attributes)

    // // 7.16
    //  void queryAttributeOwnership
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandle attribute)

    // // 7.18
    //  bool isAttributeOwnedByFederate
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandle attribute)

  void enableTimeRegulation(const rti1516::LogicalTimeInterval& logicalTimeInterval)
  {
    _ambassador->enableTimeRegulation(logicalTimeInterval);
  }

  void disableTimeRegulation()
  {
    _timeRegulationEnabled = false;
    _ambassador->disableTimeRegulation();
  }

  void enableTimeConstrained()
  {
    _ambassador->enableTimeConstrained();
  }

  void disableTimeConstrained()
  {
    _timeConstrainedEnabled = false;
    _ambassador->disableTimeConstrained();
  }

  void timeAdvanceRequest(const rti1516::LogicalTime& logicalTime)
  {
    _timeAdvancePending = true;
    _ambassador->timeAdvanceRequest(logicalTime);
  }

  void timeAdvanceRequestAvailable(const rti1516::LogicalTime& logicalTime)
  {
    _timeAdvancePending = true;
    _ambassador->timeAdvanceRequestAvailable(logicalTime);
  }

  void nextMessageRequest(const rti1516::LogicalTime& logicalTime)
  {
    _timeAdvancePending = true;
    _ambassador->nextMessageRequest(logicalTime);
  }

  void nextMessageRequestAvailable(const rti1516::LogicalTime& logicalTime)
  {
    _timeAdvancePending = true;
    _ambassador->nextMessageRequestAvailable(logicalTime);
  }

  void flushQueueRequest(const rti1516::LogicalTime& logicalTime)
  {
    // _timeAdvancePending = true;
    _ambassador->flushQueueRequest(logicalTime);
  }

  void enableAsynchronousDelivery()
  {
    _ambassador->enableAsynchronousDelivery();
  }

  void disableAsynchronousDelivery()
  {
    _ambassador->disableAsynchronousDelivery();
  }

    //  bool queryGALT (LogicalTime & logicalTime)

    //  void queryLogicalTime (LogicalTime & logicalTime)

    //  bool queryLITS (LogicalTime & logicalTime)

    //  void modifyLookahead

    //  void queryLookahead (LogicalTimeInterval & interval)

    //  void retract
    // (MessageRetractionHandle theHandle)

    //  void changeAttributeOrderType
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSet const & attributes,
    //  OrderType theType)

    //  void changeInteractionOrderType
    // (InteractionClassHandle theClass,
    //  OrderType theType)

    //  RegionHandle createRegion
    // (DimensionHandleSet const & theDimensions)

    //  void commitRegionModifications
    // (RegionHandleSet const & regionHandleSet)

    //  void deleteRegion
    // (RegionHandle region)

    //  ObjectInstanceHandle registerObjectInstanceWithRegions
    // (ObjectClassHandle theClass,
    //  AttributeHandleSetRegionHandleSetPairVector const &
    //  attributeHandleSetRegionHandleSetPairVector)

    //  ObjectInstanceHandle registerObjectInstanceWithRegions
    // (ObjectClassHandle theClass,
    //  AttributeHandleSetRegionHandleSetPairVector const &
    //  attributeHandleSetRegionHandleSetPairVector,
    //  std::wstring const & objectInstanceName)

    //  void associateRegionsForUpdates
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSetRegionHandleSetPairVector const &
    //  attributeHandleSetRegionHandleSetPairVector)

    //  void unassociateRegionsForUpdates
    // (ObjectInstanceHandle objectInstanceHandle,
    //  AttributeHandleSetRegionHandleSetPairVector const &
    //  attributeHandleSetRegionHandleSetPairVector)

    //  void subscribeObjectClassAttributesWithRegions
    // (ObjectClassHandle theClass,
    //  AttributeHandleSetRegionHandleSetPairVector const &
    //  attributeHandleSetRegionHandleSetPairVector,
    //  bool active = true)

    //  void unsubscribeObjectClassAttributesWithRegions
    // (ObjectClassHandle theClass,
    //  AttributeHandleSetRegionHandleSetPairVector const &
    //  attributeHandleSetRegionHandleSetPairVector)

    //  void subscribeInteractionClassWithRegions
    // (InteractionClassHandle theClass,
    //  RegionHandleSet const & regionHandleSet,
    //  bool active = true)

    //  void unsubscribeInteractionClassWithRegions
    // (InteractionClassHandle theClass,
    //  RegionHandleSet const & regionHandleSet)

    //  void sendInteractionWithRegions
    // (InteractionClassHandle interaction,
    //  ParameterHandleValueMap const & parameterValues,
    //  RegionHandleSet const & regionHandleSet,
    //  VariableLengthData const & tag)

    //  MessageRetractionHandle sendInteractionWithRegions
    // (InteractionClassHandle interaction,
    //  ParameterHandleValueMap const & parameterValues,
    //  RegionHandleSet const & regionHandleSet,
    //  VariableLengthData const & tag,
    //  LogicalTime const & logicalTime)

    //  void requestAttributeValueUpdateWithRegions
    // (ObjectClassHandle theClass,
    //  AttributeHandleSetRegionHandleSetPairVector const & theSet,
    //  VariableLengthData const & tag)

  rti1516::ObjectClassHandle getObjectClassHandle(std::wstring const & theName)
  {
    return _ambassador->getObjectClassHandle(theName);
  }

  std::wstring getObjectClassName(rti1516::ObjectClassHandle theHandle)
  {
    return _ambassador->getObjectClassName(theHandle);
  }

  rti1516::AttributeHandle getAttributeHandle(rti1516::ObjectClassHandle whichClass, std::wstring const & attributeName)
  {
    return _ambassador->getAttributeHandle(whichClass, attributeName);
  }

  std::wstring getAttributeName(rti1516::ObjectClassHandle whichClass, rti1516::AttributeHandle theHandle)
  {
    return _ambassador->getAttributeName(whichClass, theHandle);
  }

  rti1516::InteractionClassHandle getInteractionClassHandle(std::wstring const & theName)
  {
    return _ambassador->getInteractionClassHandle(theName);
  }

  std::wstring getInteractionClassName(rti1516::InteractionClassHandle theHandle)
  {
    return _ambassador->getInteractionClassName(theHandle);
  }

  rti1516::ParameterHandle getParameterHandle(rti1516::InteractionClassHandle whichClass, std::wstring const & theName)
  {
    return _ambassador->getParameterHandle(whichClass, theName);
  }

  std::wstring getParameterName(rti1516::InteractionClassHandle whichClass, rti1516::ParameterHandle theHandle)
  {
    return _ambassador->getParameterName(whichClass, theHandle);
  }

  rti1516::ObjectInstanceHandle getObjectInstanceHandle(std::wstring const & theName)
  {
    return _ambassador->getObjectInstanceHandle(theName);
  }

  std::wstring getObjectInstanceName(rti1516::ObjectInstanceHandle theHandle)
  {
    return _ambassador->getObjectInstanceName(theHandle);
  }

  rti1516::DimensionHandle getDimensionHandle(std::wstring const & theName)
  {
    return _ambassador->getDimensionHandle(theName);
  }

  std::wstring getDimensionName(rti1516::DimensionHandle theHandle)
  {
    return _ambassador->getDimensionName(theHandle);
  }

  unsigned long getDimensionUpperBound(rti1516::DimensionHandle theHandle)
  {
    return _ambassador->getDimensionUpperBound(theHandle);
  }

  rti1516::DimensionHandleSet getAvailableDimensionsForClassAttribute(rti1516::ObjectClassHandle theClass,
                                                                      rti1516::AttributeHandle theHandle)
  {
    return _ambassador->getAvailableDimensionsForClassAttribute(theClass, theHandle);
  }

  rti1516::ObjectClassHandle getKnownObjectClassHandle(rti1516::ObjectInstanceHandle object)
  {
    return _ambassador->getKnownObjectClassHandle(object);
  }

  rti1516::DimensionHandleSet getAvailableDimensionsForInteractionClass(rti1516::InteractionClassHandle theClass)
  {
    return _ambassador->getAvailableDimensionsForInteractionClass(theClass);
  }

  rti1516::TransportationType getTransportationType(std::wstring const & transportationName)
  {
    return _ambassador->getTransportationType(transportationName);
  }

  std::wstring getTransportationName(rti1516::TransportationType transportationType)
  {
    return _ambassador->getTransportationName(transportationType);
  }

  rti1516::OrderType getOrderType(std::wstring const & orderName)
  {
    return _ambassador->getOrderType(orderName);
  }

  std::wstring getOrderName(rti1516::OrderType orderType)
  {
    return _ambassador->getOrderName(orderType);
  }

  void enableObjectClassRelevanceAdvisorySwitch()
  {
    _ambassador->enableObjectClassRelevanceAdvisorySwitch();
  }

  void disableObjectClassRelevanceAdvisorySwitch()
  {
    _ambassador->disableObjectClassRelevanceAdvisorySwitch();
  }

  void enableAttributeRelevanceAdvisorySwitch()
  {
    _ambassador->enableAttributeRelevanceAdvisorySwitch();
  }

  void disableAttributeRelevanceAdvisorySwitch()
  {
    _ambassador->disableAttributeRelevanceAdvisorySwitch();
  }

  void enableAttributeScopeAdvisorySwitch()
  {
    _ambassador->enableAttributeScopeAdvisorySwitch();
  }

  void disableAttributeScopeAdvisorySwitch()
  {
    _ambassador->disableAttributeScopeAdvisorySwitch();
  }

  void enableInteractionRelevanceAdvisorySwitch()
  {
    _ambassador->enableInteractionRelevanceAdvisorySwitch();
  }

  void disableInteractionRelevanceAdvisorySwitch()
  {
    _ambassador->disableInteractionRelevanceAdvisorySwitch();
  }

  rti1516::DimensionHandleSet getDimensionHandleSet(rti1516::RegionHandle regionHandle)
  {
    return _ambassador->getDimensionHandleSet(regionHandle);
  }

  rti1516::RangeBounds getRangeBounds(rti1516::RegionHandle regionHandle, rti1516::DimensionHandle theDimensionHandle)
  {
    return _ambassador->getRangeBounds(regionHandle, theDimensionHandle);
  }

  void setRangeBounds(rti1516::RegionHandle regionHandle, rti1516::DimensionHandle theDimensionHandle,
                      rti1516::RangeBounds const & rangeBounds)
  {
    return _ambassador->setRangeBounds(regionHandle, theDimensionHandle, rangeBounds);
  }

  unsigned long normalizeFederateHandle(rti1516::FederateHandle federateHandle)
  {
    return _ambassador->normalizeFederateHandle(federateHandle);
  }

  unsigned long normalizeServiceGroup(rti1516::ServiceGroupIndicator theServiceGroup)
  {
    return _ambassador->normalizeServiceGroup(theServiceGroup);
  }

  bool evokeCallback(double approximateMinimumTimeInSeconds)
  {
    return _ambassador->evokeCallback(approximateMinimumTimeInSeconds);
  }

  bool evokeMultipleCallbacks(double approximateMinimumTimeInSeconds,
                              double approximateMaximumTimeInSeconds)
  {
    return _ambassador->evokeMultipleCallbacks(approximateMinimumTimeInSeconds, approximateMaximumTimeInSeconds);
  }

  void enableCallbacks()
  {
    _ambassador->enableCallbacks();
  }

  void disableCallbacks()
  {
    _ambassador->disableCallbacks();
  }

  rti1516::FederateHandle decodeFederateHandle(rti1516::VariableLengthData const& encodedValue) const
  {
    return _ambassador->decodeFederateHandle(encodedValue);
  }

  rti1516::ObjectClassHandle decodeObjectClassHandle(rti1516::VariableLengthData const & encodedValue) const
  {
    return _ambassador->decodeObjectClassHandle(encodedValue);
  }

  rti1516::InteractionClassHandle decodeInteractionClassHandle(rti1516::VariableLengthData const & encodedValue) const
  {
    return _ambassador->decodeInteractionClassHandle(encodedValue);
  }

  rti1516::ObjectInstanceHandle decodeObjectInstanceHandle(rti1516::VariableLengthData const & encodedValue) const
  {
    return _ambassador->decodeObjectInstanceHandle(encodedValue);
  }

  rti1516::AttributeHandle decodeAttributeHandle(rti1516::VariableLengthData const & encodedValue) const
  {
    return _ambassador->decodeAttributeHandle(encodedValue);
  }

  rti1516::ParameterHandle decodeParameterHandle(rti1516::VariableLengthData const & encodedValue) const
  {
    return _ambassador->decodeParameterHandle(encodedValue);
  }

  rti1516::DimensionHandle decodeDimensionHandle(rti1516::VariableLengthData const & encodedValue) const
  {
    return _ambassador->decodeDimensionHandle(encodedValue);
  }

  rti1516::MessageRetractionHandle decodeMessageRetractionHandle(rti1516::VariableLengthData const & encodedValue) const
  {
    return _ambassador->decodeMessageRetractionHandle(encodedValue);
  }

  rti1516::RegionHandle decodeRegionHandle(rti1516::VariableLengthData const & encodedValue) const
  {
    return _ambassador->decodeRegionHandle(encodedValue);
  }

protected:
  virtual void synchronizationPointRegistrationSucceeded(const std::wstring& label)
  {
  }

  virtual void synchronizationPointRegistrationFailed(const std::wstring& label, rti1516::SynchronizationFailureReason reason)
  {
  }

  virtual void announceSynchronizationPoint(const std::wstring& label, const rti1516::VariableLengthData& tag)
  {
  }

  virtual void federationSynchronized(const std::wstring& label)
  {
  }

  virtual void initiateFederateSave(const std::wstring& label)
  {
  }

  virtual void initiateFederateSave(const std::wstring& label, const rti1516::LogicalTime& logicalTime)
  {
  }

  virtual void federationSaved()
  {
  }

  virtual void federationNotSaved(rti1516::SaveFailureReason theSaveFailureReason)
  {
  }

  virtual void federationSaveStatusResponse(const rti1516::FederateHandleSaveStatusPairVector& federateStatusVector)
  {
  }

  virtual void requestFederationRestoreSucceeded(const std::wstring& label)
  {
  }

  virtual void requestFederationRestoreFailed(const std::wstring& label)
  {
  }

  virtual void federationRestoreBegun()
  {
  }

  virtual void initiateFederateRestore(const std::wstring& label, rti1516::FederateHandle handle)
  {
  }

  virtual void federationRestored()
  {
  }

  virtual void federationNotRestored(rti1516::RestoreFailureReason restoreFailureReason)
  {
  }

  virtual void federationRestoreStatusResponse(const rti1516::FederateHandleRestoreStatusPairVector& federateStatusVector)
  {
  }

  virtual void startRegistrationForObjectClass(rti1516::ObjectClassHandle)
  {
  }

  virtual void stopRegistrationForObjectClass(rti1516::ObjectClassHandle)
  {
  }

  virtual void turnInteractionsOn(rti1516::InteractionClassHandle)
  {
  }

  virtual void turnInteractionsOff(rti1516::InteractionClassHandle)
  {
  }

  virtual void objectInstanceNameReservationSucceeded(const std::wstring&)
  {
  }

  virtual void objectInstanceNameReservationFailed(const std::wstring&)
  {
  }

  virtual void discoverObjectInstance(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                      rti1516::ObjectClassHandle objectClassHandle,
                                      const std::wstring& objectInstanceName)
  {
      // ObjectClassAttributeHandleSetMap::iterator i = _subscribedObjectClassAttributeHandleSetMap.find(objectClassHandle);
      // if (i == _subscribedObjectClassAttributeHandleSetMap.end()) {
      //     fail();
      //     throw rti1516::ObjectClassNotKnown(objectClassHandle.toString());
      // }

      // if (_objectInstanceMap.find(objectInstanceHandle) != _objectInstanceMap.end()) {
      //     fail();
      //     throw rti1516::CouldNotDiscover(objectInstanceHandle.toString());
      // }

      // _objectInstanceMap[objectInstanceHandle]._objectClassHandle = objectClassHandle;
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                      const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData& tag, rti1516::OrderType, rti1516::TransportationType)
  {
      // _verifyReflectAttributeValues(objectInstanceHandle, attributeHandleValueMap);
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                      const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData&, rti1516::OrderType, rti1516::TransportationType,
                                      const rti1516::RegionHandleSet&)
  {
      // _verifyReflectAttributeValues(objectInstanceHandle, attributeHandleValueMap);
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                      const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData&, rti1516::OrderType, rti1516::TransportationType,
                                      const rti1516::LogicalTime&, rti1516::OrderType)
  {
      // _verifyReflectAttributeValues(objectInstanceHandle, attributeHandleValueMap);
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                      const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData&, rti1516::OrderType, rti1516::TransportationType,
                                      const rti1516::LogicalTime&, rti1516::OrderType, const rti1516::RegionHandleSet&)
  {
      // _verifyReflectAttributeValues(objectInstanceHandle, attributeHandleValueMap);
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                      const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData&, rti1516::OrderType, rti1516::TransportationType,
                                      const rti1516::LogicalTime&, rti1516::OrderType, rti1516::MessageRetractionHandle)
  {
      // _verifyReflectAttributeValues(objectInstanceHandle, attributeHandleValueMap);
  }

  virtual void reflectAttributeValues(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                      const rti1516::AttributeHandleValueMap& attributeHandleValueMap,
                                      const rti1516::VariableLengthData&, rti1516::OrderType, rti1516::TransportationType,
                                      const rti1516::LogicalTime&, rti1516::OrderType, rti1516::MessageRetractionHandle,
                                      const rti1516::RegionHandleSet&)
  {
      // _verifyReflectAttributeValues(objectInstanceHandle, attributeHandleValueMap);
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle, const rti1516::ParameterHandleValueMap&,
                                  const rti1516::VariableLengthData&, rti1516::OrderType, rti1516::TransportationType)
  {
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle, const rti1516::ParameterHandleValueMap&,
                                  const rti1516::VariableLengthData&, rti1516::OrderType, rti1516::TransportationType,
                                  const rti1516::RegionHandleSet&)
  {
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle interaction,
                                  rti1516::ParameterHandleValueMap const & parameterValues,
                                  rti1516::VariableLengthData const & tag,
                                  rti1516::OrderType sentOrder,
                                  rti1516::TransportationType theType,
                                  rti1516::LogicalTime const & logicalTime,
                                  rti1516::OrderType receivedOrder)
  {
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle interaction,
                                  rti1516::ParameterHandleValueMap const & parameterValues,
                                  rti1516::VariableLengthData const & tag,
                                  rti1516::OrderType sentOrder,
                                  rti1516::TransportationType theType,
                                  rti1516::LogicalTime const & logicalTime,
                                  rti1516::OrderType receivedOrder,
                                  rti1516::RegionHandleSet const & theSentRegionHandleSet)
  {
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle interaction,
                                  rti1516::ParameterHandleValueMap const & parameterValues,
                                  rti1516::VariableLengthData const & tag,
                                  rti1516::OrderType sentOrder,
                                  rti1516::TransportationType theType,
                                  rti1516::LogicalTime const & logicalTime,
                                  rti1516::OrderType receivedOrder,
                                  rti1516::MessageRetractionHandle theHandle)
  {
  }

  virtual void receiveInteraction(rti1516::InteractionClassHandle interaction,
                                  rti1516::ParameterHandleValueMap const & parameterValues,
                                  rti1516::VariableLengthData const & tag,
                                  rti1516::OrderType sentOrder,
                                  rti1516::TransportationType theType,
                                  rti1516::LogicalTime const & logicalTime,
                                  rti1516::OrderType receivedOrder,
                                  rti1516::MessageRetractionHandle theHandle,
                                  rti1516::RegionHandleSet const & theSentRegionHandleSet)
  {
  }

  virtual void removeObjectInstance(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                    rti1516::VariableLengthData const & tag,
                                    rti1516::OrderType sentOrder)
  {
      // _verifyRemoveObjectInstance(objectInstanceHandle);
  }

  virtual void removeObjectInstance(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                    rti1516::VariableLengthData const & tag,
                                    rti1516::OrderType sentOrder,
                                    rti1516::LogicalTime const & logicalTime,
                                    rti1516::OrderType receivedOrder)
  {
      // _verifyRemoveObjectInstance(objectInstanceHandle);
  }

  virtual void removeObjectInstance(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                    rti1516::VariableLengthData const & tag,
                                    rti1516::OrderType sentOrder,
                                    rti1516::LogicalTime const & logicalTime,
                                    rti1516::OrderType receivedOrder,
                                    rti1516::MessageRetractionHandle theHandle)
  {
      // _verifyRemoveObjectInstance(objectInstanceHandle);
  }

  virtual void attributesInScope(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                 rti1516::AttributeHandleSet const & attributes)
  {
  }

  virtual void attributesOutOfScope(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                    rti1516::AttributeHandleSet const & attributes)
  {
  }

  virtual void provideAttributeValueUpdate(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                           rti1516::AttributeHandleSet const & attributes,
                                           rti1516::VariableLengthData const & tag)
  {
  }

  virtual void turnUpdatesOnForObjectInstance(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                              rti1516::AttributeHandleSet const & attributes)
  {
  }

  virtual void turnUpdatesOffForObjectInstance(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                               rti1516::AttributeHandleSet const & attributes)
  {
  }

  virtual void requestAttributeOwnershipAssumption(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                                   rti1516::AttributeHandleSet const & offeredAttributes,
                                                   rti1516::VariableLengthData const & tag)
  {
  }

  virtual void requestDivestitureConfirmation(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                              rti1516::AttributeHandleSet const & releasedAttributes)
  {
  }

  virtual void attributeOwnershipAcquisitionNotification(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                                         rti1516::AttributeHandleSet const & securedAttributes,
                                                         rti1516::VariableLengthData const & tag)
  {
  }

  virtual void attributeOwnershipUnavailable(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                             rti1516::AttributeHandleSet const & attributes)
  {
  }

  virtual void requestAttributeOwnershipRelease(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                                rti1516::AttributeHandleSet const & candidateAttributes,
                                                rti1516::VariableLengthData const & tag)
  {
  }

  virtual void confirmAttributeOwnershipAcquisitionCancellation(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                                                rti1516::AttributeHandleSet const & attributes)
  {
  }

  virtual void informAttributeOwnership(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                        rti1516::AttributeHandle attribute,
                                        rti1516::FederateHandle owner)
  {
  }

  virtual void attributeIsNotOwned(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                   rti1516::AttributeHandle attribute)
  {
  }

  virtual void attributeIsOwnedByRTI(rti1516::ObjectInstanceHandle objectInstanceHandle,
                                     rti1516::AttributeHandle attribute)
  {
  }

  virtual void timeRegulationEnabled(const rti1516::LogicalTime& logicalTime)
  {
    _timeRegulationEnabled = true;
    *_grantedLogicalTime = logicalTime;
  }

  virtual void timeConstrainedEnabled(const rti1516::LogicalTime& logicalTime)
  {
    _timeConstrainedEnabled = true;
    *_grantedLogicalTime = logicalTime;
  }

  virtual void timeAdvanceGrant(const rti1516::LogicalTime& logicalTime)
  {
    _timeAdvancePending = false;
    *_grantedLogicalTime = logicalTime;
  }

  virtual void requestRetraction(rti1516::MessageRetractionHandle theHandle)
  {
  }

  // void fail()
  // { _fail = true; }

private:
  void _replaceFileWithDataIfNeeded(std::wstring& fddFile)
  {
    if (!_useDataUrlObjectModels)
      return;
    // already a data url
    if (fddFile.compare(0, 5, L"data:") == 0)
      return;
    std::wifstream stream;
    if (fddFile.compare(0, 8, L"file:///") == 0)
      stream.open(OpenRTI::ucsToLocale(fddFile.substr(8)).c_str());
    else
      stream.open(OpenRTI::ucsToLocale(fddFile).c_str());
    if (!stream.is_open())
      return;
    fddFile = L"data:,";
    std::copy(std::istreambuf_iterator<wchar_t>(stream), std::istreambuf_iterator<wchar_t>(), std::back_inserter(fddFile));
  }

  // bool _verifyGrantedLogicalTime(const rti1516::LogicalTime& logicalTime) const
  // { return logicalTime == *_grantedLogicalTime; }

  // void _verifyReflectAttributeValues(const rti1516::ObjectInstanceHandle& objectInstanceHandle,
  //                                    const rti1516::AttributeHandleValueMap& attributeHandleValueMap)
  // {
  //     ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectInstanceHandle);
  //     if (i == _objectInstanceMap.end()) {
  //         fail();
  //         throw rti1516::ObjectInstanceNotKnown(objectInstanceHandle.toString());
  //     }

  //     ObjectClassAttributeHandleSetMap::iterator j = _subscribedObjectClassAttributeHandleSetMap.find(i->second._objectClassHandle);
  //     for (rti1516::AttributeHandleValueMap::const_iterator k = attributeHandleValueMap.begin();
  //          k != attributeHandleValueMap.end(); ++k) {
  //         if (j->second.find(k->first) != j->second.end())
  //             continue;
  //         fail();
  //         throw rti1516::AttributeNotSubscribed(k->first.toString());
  //     }
  // }

  // void _verifyRemoveObjectInstance(rti1516::ObjectInstanceHandle objectInstanceHandle)
  // {
  //     if (_objectInstanceMap.find(objectInstanceHandle) == _objectInstanceMap.end()) {
  //         fail();
  //         throw rti1516::ObjectInstanceNotKnown(objectInstanceHandle.toString());
  //     }

  //     _objectInstanceMap.erase(objectInstanceHandle);
  // }

  // bool _fail;

  std::unique_ptr<rti1516::RTIambassador> _ambassador;

  bool _useDataUrlObjectModels;

  std::wstring _logicalTimeImplementationName;
  std::unique_ptr<rti1516::LogicalTimeFactory> _logicalTimeFactory;

  rti1516::FederateHandle _federateHandle;

  std::unique_ptr<rti1516::LogicalTime> _grantedLogicalTime;
  bool _timeRegulationEnabled;
  bool _timeConstrainedEnabled;
  bool _timeAdvancePending;

  // Hmm, FIXME: make an additional derived checking ambassador for the tests, keep a simple one without expensive tests
  // FIXME make this and for example the simple log below callbacks that we can attach or not as apropriate

  // // FIXME implement subscription tracking also for regions at some time
  // typedef std::map<rti1516::ObjectClassHandle, rti1516::AttributeHandleSet> ObjectClassAttributeHandleSetMap;
  // ObjectClassAttributeHandleSetMap _subscribedObjectClassAttributeHandleSetMap;

  // struct ObjectInstance {
  //   rti1516::ObjectClassHandle _objectClassHandle;
  // };

  // typedef std::map<rti1516::ObjectInstanceHandle, ObjectInstance> ObjectInstanceMap;
  // ObjectInstanceMap _objectInstanceMap;
};

}

#endif
