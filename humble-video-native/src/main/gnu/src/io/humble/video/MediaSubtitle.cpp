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
 * MediaSubtitle.cpp
 *
 *  Created on: Jul 23, 2013
 *      Author: aclarke
 */

#include "MediaSubtitle.h"
#include "MediaSubtitleImpl.h"
#include <io/humble/ferry/RefPointer.h>
#include <io/humble/ferry/Logger.h>
#include <io/humble/ferry/HumbleException.h>


using namespace io::humble::ferry;

VS_LOG_SETUP(VS_CPP_PACKAGE.MediaSubtitle);

namespace io {
namespace humble {
namespace video {

MediaSubtitle::MediaSubtitle() : mCtx(0), mComplete(false){

}

MediaSubtitle::~MediaSubtitle() {
}

MediaSubtitle*
MediaSubtitle::make(AVSubtitle* ctx)
{
  if (!ctx)
    throw HumbleInvalidArgument("no context");
  RefPointer<MediaSubtitle> retval = MediaSubtitle::make();
  retval->mCtx = ctx;
  return retval.get();
}

MediaSubtitleRectangle*
MediaSubtitle::getRectangle(int32_t n)
{
  if (n < 0 || n >= (int32_t)mCtx->num_rects)
    throw HumbleInvalidArgument("attempt to get out-of-range rectangle");

  if (!mCtx->rects)
    throw HumbleRuntimeError("no rectangles");

  return MediaSubtitleRectangle::make(mCtx->rects[n]);
}

MediaSubtitleRectangle*
MediaSubtitleRectangle::make(AVSubtitleRect* ctx) {
  if (!ctx)
    throw HumbleInvalidArgument("no context");
  RefPointer<MediaSubtitleRectangle> retval = make();
  retval->mCtx = ctx;
  return retval.get();
}
int32_t
MediaSubtitleRectangle::getPictureLinesize(int line) {
  if (line < 0 || line >= 4)
    throw HumbleInvalidArgument("line must be between 0 and 3");
  return mCtx->linesize[line];
}
Buffer*
MediaSubtitleRectangle::getPictureData(int line)
{
  if (line < 0 || line >= 4)
    throw HumbleInvalidArgument("line must be between 0 and 3");
  // add ref ourselves for the Buffer
  this->acquire();
  // create a buffer
  RefPointer<Buffer> retval = Buffer::make(this, mCtx->data[line], mCtx->linesize[line],
      Buffer::refCountedFreeFunc, this);
  if (!retval)
    this->release();
  return retval.get();
}

int64_t
MediaSubtitle::logMetadata(char * buffer, size_t len) {
  VS_THROW(HumbleRuntimeError("NOT IMPLEMENTED"));
  return snprintf(buffer, len, "NOT IMPLEMENTED");
}

} /* namespace video */
} /* namespace humble */
} /* namespace io */
