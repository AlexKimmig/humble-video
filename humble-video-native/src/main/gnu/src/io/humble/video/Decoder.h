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
 * Decoder.h
 *
 *  Created on: Jul 23, 2013
 *      Author: aclarke
 */

#ifndef DECODER_H_
#define DECODER_H_

#include <io/humble/ferry/RefPointer.h>
#include <io/humble/video/Coder.h>
#include <io/humble/video/Processor.h>
#include <io/humble/video/MediaPacket.h>
#include <io/humble/video/MediaAudio.h>
#include <io/humble/video/MediaPicture.h>
#include <io/humble/video/MediaSubtitle.h>

namespace io {
namespace humble {
namespace video {

/**
 * Decodes MediaPacket objects into MediaAudio, MediaPicture or MediaSubtitle objects.
 */
class VS_API_HUMBLEVIDEO Decoder :
  virtual public io::humble::video::Coder,
  virtual public io::humble::video::ProcessorEncodedSink,
  virtual public io::humble::video::ProcessorRawSource
{
public:
  /**
   * Create a Decoder that will use the given Codec.
   *
   * @return a Decoder
   * @throws InvalidArgument if codec is null or codec cannot decode.
   */
  static Decoder* make(Codec* codec);

  /**
   * Creates a Decoder, copying parameters from a given Coder (either an encoder or a decoder).
   * @return a Decoder
   * @throws InvalidArgument if src is null
   */
  static Decoder* make(Coder* src);

#ifndef SWIG
  static Decoder* make(const AVCodec* codec, const AVCodecParameters *src);
#endif // SWIG

#if VS_OLD_DECODE_API
  /**
   * Flush this Decoder, getting rid of any cached packets (call after seek).
   * Next packet given to decode should be a key packet.
   */
  virtual void flush();

  /**
   * Decode this packet into output.  It will
   * try to fill up the audio samples object, starting
   * from the byteOffset inside this packet.
   * <p>
   * The caller is responsible for allocating the
   * MediaAudio object.  This function will overwrite
   * any data in the samples object.
   * </p>
   * @param output The MediaAudio we decode to. Caller must check if it is complete on return.
   * @param packet    The packet we're attempting to decode from.
   * @param byteOffset Where in the packet payload to start decoding
   *
   * @return number of bytes actually processed from the packet, or negative for error
   */
  virtual int32_t decodeAudio(MediaAudio * output,
      MediaPacket *packet, int32_t byteOffset);

  /**
   * Decode this packet into output.
   *
   * The caller is responsible for allocating the
   * MediaPicture object.  This function will potentially
   * overwrite any data in the frame object, but
   * you should pass the same MediaPicture into this function
   * repeatedly until Media.isComplete() is true.
   * <p>
   * Note on memory for MediaPicture: For a multitude of reasons,
   * if you created MediaPicture from a buffer, decodeVideo will discard
   * it and replace it with a buffer that is aligned correctly for different
   * CPUs and different codecs. If you must have a copy of the image data
   * in memory managed by you, then pass in a MediaPicture allocated without
   * a buffer to DecodeVideo, and then copy that into your own media picture.
   * </p>
   *
   * @param output The MediaPicture we decode. Caller must check if it is complete on return.
   * @param packet  The packet we're attempting to decode from.
   * @param byteOffset Where in the packet payload to start decoding
   *
   * @return number of bytes actually processed from the packet, or negative for error
   */
  virtual int32_t decodeVideo(MediaPicture * output,
      MediaPacket *packet, int32_t byteOffset);

  /**
   * Decode this packet into output.  It will
   * try to fill up the media object, starting
   * from the byteOffset inside this packet.
   * <p>
   * The caller is responsible for allocating the
   * correct underlying Media object.  This function will overwrite
   * any data in the samples object.
   * </p>
   * @param output The Media we decode to. Caller must check if it is complete on return.
   * @param packet    The packet we're attempting to decode from.
   * @param byteOffset Where in the packet payload to start decoding
   *
   * @return number of bytes actually processed from the packet, or negative for error
   *
   * @throws InvalidArgument if the media type is not compatible with this decoder.
   * @see decodeVideo
   * @see decodeAudio
   */
  virtual int32_t decode(MediaSampled * output,
      MediaPacket *packet, int32_t byteOffset);
#endif // VS_OLD_DECODE_API


  /**
   * Decode this packet into output.
   *
   * The caller is responsible for allocating the
   * MediaSubtitle object.  This function will potentially
   * overwrite any data in the frame object, but
   * you should pass the same MediaSubtitle into this function
   * repeatedly until Media.isComplete() is true.
   *
   * @param output The MediaPicture we decode. Caller must check if it is complete on return.
   * @param packet  The packet we're attempting to decode from.
   * @param byteOffset Where in the packet payload to start decoding
   *
   * @return number of bytes actually processed from the packet, or negative for error
   */
//  virtual int32_t decodeSubtitle(MediaSubtitle * output,
//      MediaPacket *packet, int32_t byteOffset);


  virtual ProcessorResult send(MediaEncoded*);
  virtual ProcessorResult receive(MediaRaw*);


protected:
  Decoder(const AVCodec* codec, const AVCodecParameters* src);
  virtual ~Decoder();

#if VS_OLD_DECODE_API
  virtual int prepareFrame(AVFrame* frame, int flags);
#endif // VS_OLD_DECODE_API

private:
  int64_t rebase(int64_t ts, MediaPacket* packet);
  io::humble::ferry::RefPointer<MediaRaw> mCachedMedia;
  int64_t mAudioDiscontinuityStartingTimeStamp;
  int64_t mSamplesSinceLastTimeStampDiscontinuity;
};

} /* namespace video */
} /* namespace humble */
} /* namespace io */
#endif /* DECODER_H_ */
