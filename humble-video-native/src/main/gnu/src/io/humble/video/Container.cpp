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

#include "Container.h"
#include "PropertyImpl.h"
#include "Encoder.h"
#include "Decoder.h"
#include <io/humble/ferry/Logger.h>
#include <io/humble/ferry/RefPointer.h>
#include <io/humble/video/customio/URLProtocolHandler.h>

VS_LOG_SETUP(VS_CPP_PACKAGE.Container);

using namespace io::humble::video::customio;
using namespace io::humble::ferry;

namespace io {
namespace humble {
namespace video {

Container::Container() {
  VS_LOG_TRACE("Created: %p");
}

Container::~Container() {
  // Remove all the stream meta-data objects.
  while(mStreams.size() > 0)
  {
    Stream* stream=mStreams.back();
    mStreams.pop_back();
    if (stream) {
      delete stream;
    }
  }
  VS_LOG_TRACE("Destroyed: %p");
}

int
Container::url_read(void*h, unsigned char* buf, int size) {
  int retval = -1;
  try {
    URLProtocolHandler* handler = (URLProtocolHandler*) h;
    if (handler) retval = handler->url_read(buf, size);
  } catch (...) {
    retval = -1;
  }
  VS_LOG_TRACE("URLProtocolHandler[%p]->url_read(%p, %d) ==> %d", h, buf, size,
      retval);
  return retval;
}
int
Container::url_write(void*h, unsigned char* buf, int size) {
  int retval = -1;
  try {
    URLProtocolHandler* handler = (URLProtocolHandler*) h;
    if (handler) retval = handler->url_write(buf, size);
  } catch (...) {
    retval = -1;
  }
  VS_LOG_TRACE("URLProtocolHandler[%p]->url_write(%p, %d) ==> %d", h, buf, size,
      retval);
  return retval;
}

int64_t
Container::url_seek(void*h, int64_t position, int whence) {
  int64_t retval = -1;
  try {
    URLProtocolHandler* handler = (URLProtocolHandler*) h;
    if (handler) retval = handler->url_seek(position, whence);
  } catch (...) {
    retval = -1;
  }
  VS_LOG_TRACE("URLProtocolHandler[%p]->url_seek(%p, %lld) ==> %d", h, position,
      whence, retval);
  return retval;
}

Container::Stream::Stream(Container* container, int32_t index) {
  mContainer = container;
  mIndex = index;
  mLastDts = Global::NO_PTS;
  mCtx = mContainer->getFormatCtx()->streams[index];
}

Container::Stream::~Stream() {
}

Coder*
Container::Stream::getCoder() {
  // we need to know the stream direction...
  AVFormatContext* ctx = mContainer->getFormatCtx();
  AVStream* stream = getCtx();

  if (!ctx || !stream) {
    VS_THROW(HumbleRuntimeError("could not get container context to find coder"));
  }
  if (!mCoder && ctx->oformat) {
      VS_THROW(HumbleRuntimeError("Got null encoder on MuxerStream which should not be possible"));
  }
  return mCoder.get();
}

void
Container::doSetupStreams() {
  // do nothing if we're already all set up.
  AVFormatContext* ctx = getFormatCtx();
  if (!ctx) {
    VS_THROW(HumbleRuntimeError("Attempt to setup streams on closed or error container"));
  }
  if (mStreams.size() == ctx->nb_streams) return;

  // Only look for new streams
  for (uint32_t i = mStreams.size(); i < ctx->nb_streams; i++) {
    AVStream *avStream = ctx->streams[i];
    if (!avStream)
      VS_THROW(HumbleRuntimeError::make("no FFMPEG allocated stream: %d", i));

    // now let's initialize our stream object.
    Stream* stream = new Stream(this, i);
    mStreams.push_back(stream);
    if (ctx->iformat) {
      // if a input format, we try to set up the decoder objects here. Some streams
      // will have no decoder objects available.
      RefPointer<Codec> codec = Codec::findDecodingCodec((Codec::ID)avStream->codecpar->codec_id);
      RefPointer<Coder> coder;
      if (codec) {
        // make a copy of the decoder so we decouple it from the container
        // completely
        coder = Decoder::make(codec->getCtx(), avStream->codecpar);
        stream->setCoder(coder.value());
      } else {
        if (avStream->codecpar->codec_id != AV_CODEC_ID_NONE)
          VS_LOG_DEBUG("noDecoderAvailable Container@%p[i=%"PRId32";codec_id:%"PRId32"];",
                       this,
                       avStream->index,
                       avStream->codecpar->codec_id);
      }
#ifdef VS_DEBUG
      VS_LOG_TRACE("newStreamFound Container@%p[i=%"PRId32";c=%p;tb=%"PRId32"/%"PRId32";]",
                   this,
                   (int32_t)avStream->index,
                   coder.value(),
                   (int32_t)avStream->time_base.num,
                   (int32_t)avStream->time_base.den);
#endif
    }
  }
}

Container::Stream*
Container::getStream(int32_t index) {
  if (index < 0) {
    VS_THROW(HumbleInvalidArgument("index must be >= 0"));
  }
  doSetupStreams();
  if ((size_t)index >= mStreams.size()) {
    VS_THROW(HumbleInvalidArgument("index must be < #getNumStreams()"));
  }
  return mStreams[index];
}

int32_t
Container::getNumStreams()
{
  doSetupStreams();
  return mStreams.size();
}

} /* namespace video */
} /* namespace humble */
} /* namespace io */
