/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

void Analytics::addDestination (AnalyticsDestination* destination)
{
    destinations.add (destination);
}

OwnedArray<AnalyticsDestination>& Analytics::getDestinations()
{
    return destinations;
}

void Analytics::setUserId (String newUserId)
{
    userId = newUserId;
}

void Analytics::setUserProperties (StringPairArray properties)
{
    userProperties = properties;
}

void Analytics::logEvent (const String& eventName,
                          const StringPairArray& parameters,
                          int eventType)
{
    if (! isSuspended)
    {
        AnalyticsDestination::AnalyticsEvent event
        {
            eventName,
            eventType,
            Time::getMillisecondCounter(),
            parameters,
            userId,
            userProperties
        };

        for (auto* destination : destinations)
            destination->logEvent (event);
    }
}

void Analytics::setSuspended (bool shouldBeSuspended)
{
    isSuspended = shouldBeSuspended;
}

JUCE_IMPLEMENT_SINGLETON (Analytics)

}
