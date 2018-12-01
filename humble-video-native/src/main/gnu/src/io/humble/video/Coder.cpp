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
 * Coder.cpp
 *
 *  Created on: Jul 23, 2013
 *      Author: aclarke
 */

#include "Coder.h"
#include <io/humble/ferry/Logger.h>
#include <io/humble/ferry/HumbleException.h>
#include <io/humble/video/KeyValueBagImpl.h>
#include <io/humble/video/MediaAudio.h>
#include <io/humble/video/MediaPicture.h>

VS_LOG_SETUP(VS_CPP_PACKAGE.Coder);

namespace io {
namespace humble {
namespace video {

using namespace io::humble::ferry;

Coder::Coder(const AVCodec* codec, const AVCodecParameters* src) {
  mCtx = avcodec_alloc_context3(codec);
  if (!mCtx)
    throw HumbleRuntimeError("could not allocate coder context");
  if (src && avcodec_parameters_to_context(mCtx, src) < 0) {
    throw HumbleRuntimeError("Could not copy source context");
  }
  mCodec = Codec::make(mCtx->codec);

  // set fields we override/use
  mCtx->get_buffer2 = Coder::getBuffer;
  mCtx->opaque = this;

  mState = STATE_INITED;

  VS_LOG_TRACE("Created: %p", this);

}

Coder::~Coder() {
  VS_LOG_TRACE("Destroyed: %p", this);
  (void) avcodec_close(mCtx);
  if (mCtx->extradata)
    av_freep(&mCtx->extradata);
  av_freep(&mCtx);
}

void
Coder::setState(State state) {
  const char* descrs[4];
  (void) descrs; // because VS_LOG_TRACE gets compiled away in debug builds.
  descrs[STATE_INITED] = "STATE_INITED";
  descrs[STATE_OPENED] = "STATE_OPENED";
  descrs[STATE_FLUSHING] = "STATE_FLUSHING";
  descrs[STATE_ERROR] = "STATE_ERROR";
  VS_LOG_TRACE("setState Coder@%p[%s -> %s]",
               this,
               descrs[mState],
               descrs[state]);
  mState = state;
}

void
Coder::open(KeyValueBag* inputOptions, KeyValueBag* aUnsetOptions) {
  int32_t retval = -1;
  AVDictionary* tmp=0;
  try {
      // Time to set options
    if (inputOptions) {
      KeyValueBagImpl* options = dynamic_cast<KeyValueBagImpl*>(inputOptions);
      // make a copy of the data returned.
      av_dict_copy(&tmp, options->getDictionary(), 0);
    }

    // first we're going to set options (and we'll set them again later)
    retval = av_opt_set_dict(mCtx, &tmp);
    FfmpegException::check(retval, "could not set options on coder");

    // we check that the options passed in our valid
    checkOptionsBeforeOpen();

    RefPointer<Codec> codec = getCodec();
    // we pass in the options again because codec-specific options can be set.
    retval = avcodec_open2(mCtx, codec->getCtx(), &tmp);
    FfmpegException::check(retval, "could not open codec");
    setState(STATE_OPENED);

    if (aUnsetOptions)
    {
      KeyValueBagImpl* unsetOptions = dynamic_cast<KeyValueBagImpl*>(aUnsetOptions);
      unsetOptions->copy(tmp);
    }
    if (tmp)
      av_dict_free(&tmp);
  } catch (...) {
    setState(STATE_ERROR);
    if (tmp)
      av_dict_free(&tmp);
    throw;
  }
}


Rational*
Coder::getTimeBase() {
  if (!mTimebase || mTimebase->getNumerator() != mCtx->time_base.num || mTimebase->getDenominator() != mCtx->time_base.den)
    mTimebase = Rational::make(mCtx->time_base.num, mCtx->time_base.den);
  return mTimebase.get();
}

void
Coder::setTimeBase(Rational* newTimeBase) {
  if (!newTimeBase)
    throw HumbleInvalidArgument("no timebase passed in");
  if (mState != STATE_INITED)
    throw HumbleRuntimeError("can only setTimeBase on Decoder before open() is called.");
  mTimebase.reset(newTimeBase, true);
  mCtx->time_base.num = newTimeBase->getNumerator();
  mCtx->time_base.den = newTimeBase->getDenominator();
}

int
Coder::getBuffer(struct AVCodecContext *s, AVFrame *frame, int flags) {
  Coder* coder = static_cast<Coder*>(s->opaque);
  if (!coder)
    return avcodec_default_get_buffer2(s, frame, flags);

  if (!(coder->mCodec->getCapabilities() & Codec::CAP_DR1))
    return avcodec_default_get_buffer2(s, frame, flags);

  return coder->prepareFrame(frame, flags);
}

int32_t
Coder::getFrameSize() {
  int32_t retval = (int32_t) getPropertyAsLong("frame_size");
  if (retval < 0)
    return retval;
  RefPointer<Codec> codec = getCodec();
  if (codec->getType() == MediaDescriptor::MEDIA_AUDIO)
  {
    if (retval <= 1)
    {
      // Rats; some PCM encoders give a frame size of 1, which is too
      //small.  We pick a more sensible value.
      retval = 576;
    }
  }
  return retval;
}

void
Coder::ensurePictureParamsMatch(MediaPicture* pict)
{
  if (!pict)
    return;

  if (getWidth() != pict->getWidth())
    VS_THROW(HumbleInvalidArgument("width on picture does not match what coder expects"));

  if (getHeight() != pict->getHeight())
    VS_THROW(HumbleInvalidArgument("height on picture does not match what coder expects"));

  if (getPixelFormat() != pict->getFormat())
    VS_THROW(HumbleInvalidArgument("Pixel format on picture does not match what coder expects"));

}

void
Coder::ensureAudioParamsMatch(MediaAudio* audio)
{
  if (!audio)
    return;

  if (getChannels() != audio->getChannels())
    VS_THROW(HumbleInvalidArgument("audio channels does not match what coder expects"));

  if (getSampleRate() != audio->getSampleRate())
    VS_THROW(HumbleInvalidArgument("audio sample rate does not match what coder expects"));

  if (getSampleFormat() != audio->getFormat())
    VS_THROW(HumbleInvalidArgument::make("audio sample format does not match what coder expects: %d vs %d",
        getSampleFormat(), audio->getFormat()));

}

int32_t
Coder::getFlags()
{
  return mCtx->flags;
}
int32_t
Coder::getFlag(Flag flag) {
  return mCtx->flags & flag;
}
int32_t
Coder::getFlags2()
{
  return mCtx->flags2;
}
int32_t
Coder::getFlag2(Flag2 flag) {
  return mCtx->flags2 & flag;
}
void
Coder::setFlags(int32_t val)
{
  if (getState() != STATE_INITED)
    VS_THROW(HumbleInvalidArgument("Cannot set flags after coder is opened"));

  mCtx->flags = val;
}
void
Coder::setFlag(Flag flag, bool value)
{
  if (getState() != STATE_INITED)
    VS_THROW(HumbleInvalidArgument("Cannot set flags after coder is opened"));

  if (value)
  {
    mCtx->flags |= flag;
  }
  else
  {
    mCtx->flags &= (~flag);
  }
}
void
Coder::setFlags2(int32_t val)
{
  if (getState() != STATE_INITED)
    VS_THROW(HumbleInvalidArgument("Cannot set flags after coder is opened"));

  mCtx->flags2 = val;
}
void
Coder::setFlag2(Flag2 flag, bool value)
{
  if (getState() != STATE_INITED)
    VS_THROW(HumbleInvalidArgument("Cannot set flags after coder is opened"));

  if (value)
  {
    mCtx->flags2 |= flag;
  }
  else
  {
    mCtx->flags2 &= (~flag);
  }
}

MediaParameters*
Coder::getMediaParameters()
{
  AVCodecParameters* p = avcodec_parameters_alloc();
  RefPointer<MediaParameters> retval = 0;
  try {
    if (!p)
      VS_THROW(HumbleBadAlloc());
    // copy parameters in
    if (avcodec_parameters_from_context(p, mCtx)<0) {
      avcodec_parameters_free(&p);
      VS_THROW(HumbleBadAlloc());
    }
    retval = MediaParameters::make(p, mTimebase.value());
    avcodec_parameters_free(&p);
  } catch (...) {
    // technically on a bad alloc we'll leak memory here. Should catch
    // and rethrow. I hate exceptions.
    avcodec_parameters_free(&p);
    throw;
  }

  return retval.get();
}

void
Coder::setMediaParameters(MediaParameters* p) {
  if (getState() != STATE_INITED)
    VS_THROW(HumbleRuntimeError("cannot set parameters after code is opened"));
  if (!p)
    VS_THROW(HumbleInvalidArgument("must pass in non null parameters"));

  int e = avcodec_parameters_to_context(mCtx, p->getCtx());
  if (e < 0) {
    setState(STATE_ERROR);
    FfmpegException::check(e, "cannot set parameters, unknown error occurred:");
  }
  RefPointer<Rational> tb = p->getTimeBase();
  mTimebase = tb.value();
}


} /* namespace video */
} /* namespace humble */
} /* namespace io */
