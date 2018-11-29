/*******************************************************************************
 * Copyright (c) 2014, Andrew "Art" Clarke.  All rights reserved.
 *   
 * This file is part of Humble-Video.
 *
 * Humble-Video is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Humble-Video is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Humble-Video.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/
/*
 * FilterSink.cpp
 *
 *  Created on: Aug 5, 2013
 *      Author: aclarke
 */

#include "FilterSink.h"
#include <io/humble/ferry/RefPointer.h>
#include <io/humble/ferry/Logger.h>
#include <io/humble/video/VideoExceptions.h>
#include <io/humble/video/FilterGraph.h>

using namespace io::humble::ferry;

VS_LOG_SETUP(VS_CPP_PACKAGE.FilterSink);

namespace io {
namespace humble {
namespace video {

FilterSink::FilterSink(FilterGraph* graph, AVFilterContext* ctx) :
    FilterEndPoint(graph, ctx) {
}

FilterSink::~FilterSink() {
}

ProcessorResult
FilterSink::sendRaw(MediaRaw* media) {
  // ok, let's get to work
  AVFilterContext* ctx = getFilterCtx();
  AVFrame* frame = 0;
  if (media) {
    if (!media->isComplete()) {
      VS_THROW(HumbleInvalidArgument("incomplete media passed in"));
    }
    frame = media->getCtx();
  }

  int e = av_buffersrc_write_frame(ctx, frame);
  if (e != AVERROR_EOF && e != AVERROR(EAGAIN))
    FfmpegException::check(e, "could not add frame to filter sink: ");
  return (ProcessorResult) e;
}


} /* namespace video */
} /* namespace humble */
} /* namespace io */
