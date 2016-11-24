/// \file AliO2Timeframe.cxx
/// \brief implementation of the AliO2Timeframe class.
/// \since 2016-11-21
/// \author R.G.A. Deckers
/// \copyright
///  This program is free software; you can redistribute it and/or
/// modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 3 of
/// the License, or (at your option) any later version.
/// This program is distributed in the hope that it will be useful, but
/// WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
/// General Public License for more details at
/// https://www.gnu.org/copyleft/gpl.html

#include "AliO2Timeframe.h"
#include "AliO2Event.h"
#include <TRandom2.h>
#include <algorithm>
// root specific
ClassImp(AliO2Timeframe);

// default constructor
AliO2Timeframe::AliO2Timeframe() {}

// default destructor
AliO2Timeframe::~AliO2Timeframe() {}

/// Produces an event corresponding to the 'index'ed vertex.
///
/// It's not very pretty right now, or optimized but it is for testing purposes.
/// \param index The index of the corresponding primary vertex. is not bounds
/// checked
/// \return Returns an AliO2Event object which consists of pointers into this
/// timeframe.
AliO2Event AliO2Timeframe::getEvent(size_t index) {
  // TODO: bounds checking
  AliO2Vertex *vertex = mVertices.data() + index;
  timestamp_t eventTime = vertex->getTimestamp();
  AliO2GlobalTrack *global_track_array = NULL;
  size_t size_global = 0;
  size_t offset_unambigous_global = 0;
  size_t size_unambigous_global = 0;
  if (mGlobalTracks.size()) {
    timestamp_t eventTimeDelta =
        vertex->getTimestampResolution() +
        mGlobalTracks.begin()->getTimestampResolution();
    // TODO:  ROOT6 - > auto
    typedef std::vector<AliO2GlobalTrack>::iterator globalIt_t;
    // Points to the first one
    globalIt_t globalItBegin = std::lower_bound(
        mGlobalTracks.begin(), mGlobalTracks.end(), eventTime - eventTimeDelta);
    // points to the first element that's *above* threshold (or last element if
    // none exist)
    globalIt_t globalItEnd = std::upper_bound(
        globalItBegin, mGlobalTracks.end(), eventTime + eventTimeDelta);
    globalIt_t globalItUnambigousBegin = globalItBegin;
    globalIt_t globalItUnambigousEnd = globalItEnd;
    // if there is is an event before us check when it overlaps
    if (0 != index) {
      AliO2Vertex *vertex = mVertices.data() + index - 1;
      timestamp_t eventTime = vertex->getTimestamp();
      timestamp_t eventTimeDelta =
          vertex->getTimestampResolution() +
          mGlobalTracks.begin()->getTimestampResolution();
      // find the overlapping bound, points to the element past
      globalItUnambigousBegin = std::upper_bound(globalItBegin, globalItEnd,
                                                 eventTime + eventTimeDelta);
    }
    // if there is an event after this one in this frame
    if (mVertices.size() - 1 != index) {
      AliO2Vertex *vertex = mVertices.data() + index + 1;
      timestamp_t eventTime = vertex->getTimestamp();
      timestamp_t eventTimeDelta =
          vertex->getTimestampResolution() +
          mGlobalTracks.begin()->getTimestampResolution();
      globalItUnambigousEnd = std::lower_bound(globalItBegin, globalItEnd,
                                               eventTime - eventTimeDelta);
    }
    // TODO: when begin points past end?
    // Now we have 4 iterators, identifying the slice of associated tracks, and
    // the slice which is unambigously mapped to us.
    global_track_array = &(*globalItBegin);
    size_global = std::distance(globalItBegin, globalItEnd);
    offset_unambigous_global =
        std::distance(globalItBegin, globalItUnambigousBegin);
    size_unambigous_global =
        std::distance(globalItUnambigousBegin, globalItUnambigousEnd);
  }

  AliO2ITSTrack *ITS_track_array = NULL;
  size_t size_ITS = 0;
  size_t offset_unambigous_ITS = 0;
  size_t size_unambigous_ITS = 0;
  if (mITSTracks.size()) {
    timestamp_t eventTimeDelta = vertex->getTimestampResolution() +
                                 mITSTracks.begin()->getTimestampResolution();
    // TODO:  ROOT6 - > auto

    typedef std::vector<AliO2ITSTrack>::iterator ITSIt_t;
    // Points to the first one
    ITSIt_t ITSItBegin = std::lower_bound(mITSTracks.begin(), mITSTracks.end(),
                                          eventTime - eventTimeDelta);
    // points to the first element that's *above* threshold (or last element if
    // none exist)
    ITSIt_t ITSItEnd = std::upper_bound(ITSItBegin, mITSTracks.end(),
                                        eventTime + eventTimeDelta);
    ITSIt_t ITSItUnambigousBegin = ITSItBegin;
    ITSIt_t ITSItUnambigousEnd = ITSItEnd;
    // if there is is an event before us check when it overlaps
    if (0 != index) {
      AliO2Vertex *vertex = mVertices.data() + index - 1;
      timestamp_t eventTime = vertex->getTimestamp();
      timestamp_t eventTimeDelta = vertex->getTimestampResolution() +
                                   mITSTracks.begin()->getTimestampResolution();
      // find the overlapping bound, points to the element past
      ITSItUnambigousBegin =
          std::upper_bound(ITSItBegin, ITSItEnd, eventTime + eventTimeDelta);
    }
    // if there is an event after this one in this frame
    if (mVertices.size() - 1 != index) {
      AliO2Vertex *vertex = mVertices.data() + index + 1;
      timestamp_t eventTime = vertex->getTimestamp();
      timestamp_t eventTimeDelta = vertex->getTimestampResolution() +
                                   mITSTracks.begin()->getTimestampResolution();
      ITSItUnambigousEnd =
          std::lower_bound(ITSItBegin, ITSItEnd, eventTime - eventTimeDelta);
    }
    // TODO: when begin points past end?
    // Now we have 4 iterators, identifying the slice of associated tracks, and
    // the
    // slice which is unambigously mapped to us.
    ITS_track_array = &(*ITSItBegin);
    size_ITS = std::distance(ITSItBegin, ITSItEnd);
    offset_unambigous_ITS = std::distance(ITSItBegin, ITSItUnambigousBegin);
    size_unambigous_ITS =
        std::distance(ITSItUnambigousBegin, ITSItUnambigousEnd);
  }
  // ugly... just like that ^^^^``
  return AliO2Event(vertex, global_track_array, size_global,
                    offset_unambigous_global, size_unambigous_global,
                    ITS_track_array, size_ITS, offset_unambigous_ITS,
                    size_unambigous_ITS);
}

/// Converts and adds an old ESD event to the timeframe with the given timestamp
void AliO2Timeframe::addEvent(const AliESDEvent *event,
                              timestamp_t timestampNs) {
  // Random number generator for timestamps
  TRandom2 rng;
  int numberOfTracks = event->GetNumberOfTracks();
  // if we have no tracks in this event don't bother
  if (0 == numberOfTracks) {
    return;
  }
  // TODO: ROOT6 -> auto
  size_t globalTracksOldSize = mGlobalTracks.size();
  size_t ITSTracksOldSize = mITSTracks.size();
  size_t verticesOldSize = mVertices.size();

  // Get the primary vertex of this event
  mVertices.push_back(AliO2Vertex(
      event->GetVertex(), rng.Gaus(timestampNs, 100.0 / sqrt(numberOfTracks)),
      600.0 / sqrt(numberOfTracks))); // resolution of 6 sigma
  // get all the tracks
  for (int i = 0; i < numberOfTracks; i++) {
    AliESDtrack *track = event->GetTrack(i);
    // Check if this track is global or ITS
    if (NULL == track->GetInnerParam()) {
      // ITS track, box the time in bins of 5000ns and assign the central time
      mITSTracks.push_back(
          AliO2ITSTrack(track, 5000.0 * floor(timestampNs / 5000.0) + 2500));
    } else {
      // global track, guass of width 100ns
      mGlobalTracks.push_back(
          AliO2GlobalTrack(track, rng.Gaus(timestampNs, 100)));
    }
  }
  // TODO: vertices shouldn't really need to be sorted, only one can get added
  // per function call anyway
  // TODO: write a fast sorter which take the Guassian nature into account.

  // Sort the new slices
  std::sort(mGlobalTracks.begin() + globalTracksOldSize, mGlobalTracks.end());
  // merge the old and new slices
  std::inplace_merge(mGlobalTracks.begin(),
                     mGlobalTracks.begin() + globalTracksOldSize,
                     mGlobalTracks.end());
  std::sort(mITSTracks.begin() + ITSTracksOldSize, mITSTracks.end());
  std::inplace_merge(mITSTracks.begin(), mITSTracks.begin() + ITSTracksOldSize,
                     mITSTracks.end());
  std::sort(mVertices.begin() + verticesOldSize, mVertices.end());
  std::inplace_merge(mVertices.begin(), mVertices.begin() + verticesOldSize,
                     mVertices.end());
}