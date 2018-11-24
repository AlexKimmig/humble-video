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

#ifndef CONTAINERFORMAT_H_
#define CONTAINERFORMAT_H_

#include <io/humble/ferry/RefCounted.h>
#include <io/humble/video/HumbleVideo.h>
#include <io/humble/video/Codec.h>

namespace io {
namespace humble {
namespace video {

/**
 * Parent class for objects that describe different container formats
 * for media (e.g. MP4 vs. FLV).
 */
class VS_API_HUMBLEVIDEO ContainerFormat : public io::humble::ferry::RefCounted
{
public:
  /**
   * A series of flags that different ContainerFormats and their subclasses
   * can support.
   */
  typedef enum Flag
  {
    INVALID_FLAG = -1,

    /**
     * This format does not use an on-disk file (e.g. a network format)
     */
    NO_FILE = AVFMT_NOFILE,
    /** Needs a percent-d in filename. */
    NEED_NUMBER = AVFMT_NEEDNUMBER,

    /** Show format stream IDs numbers. */
    SHOW_IDS = AVFMT_SHOW_IDS,

    /** Format wants global header. */
    GLOBAL_HEADER = AVFMT_GLOBALHEADER,

    /** Format does not need / have any timestamps. */
    NO_TIMESTAMPS = AVFMT_NOTIMESTAMPS,

    /** Use generic index building code. */
    GENERIC_INDEX = AVFMT_GENERIC_INDEX,

    /** Format allows timestamp discontinuities. Note, muxers always require valid (monotone) timestamps */
    TIMESTAMP_DISCONTINUITIES = AVFMT_TS_DISCONT,

    /** Format allows variable fps. */
    VARIABLE_FPS = AVFMT_VARIABLE_FPS,

    /** Format does not need width/height */
    NO_DIMENSIONS = AVFMT_NODIMENSIONS,

    /** Format does not require any streams */
    NO_STREAMS = AVFMT_NOSTREAMS,

    /** Format does not allow to fallback to binary search via read_timestamp */
    NO_BINARY_SEARCH = AVFMT_NOBINSEARCH,

    /** Format does not allow to fallback to generic search */
    NO_GENERIC_SEARCH = AVFMT_NOGENSEARCH,

    /** Format does not allow seeking by bytes */
    NO_BYTE_SEEKING = AVFMT_NO_BYTE_SEEK,

    /** Format allows flushing. If not set, the muxer will not receive a NULL packet in the write_packet function. */
    ALLOW_FLUSH = AVFMT_ALLOW_FLUSH,

    /** Format does not require strictly increasing timestamps, but they must still be monotonic */
    NONSTRICT_TIMESTAMPS = AVFMT_TS_NONSTRICT,

    /** Format allows muxing negative timestamps. If not set the timestamp
         will be shifted in av_write_frame and av_interleaved_write_frame so they
         start from 0. The user or muxer can override this through AVFormatContext.avoid_negative_ts
     */
    TS_NEGATIVE = AVFMT_TS_NEGATIVE,

    /** Seeking is based on PTS */
    SEEK_TO_PTS = AVFMT_SEEK_TO_PTS,
  } Flag;

  /**
   * Name for format.
   */
  virtual const char*
  getName()=0;

  /**
   * Descriptive name for the format, meant to be more human-readable
   * than name.
   */
  virtual const char*
  getLongName()=0;

  /** A comma-separated list of supported filename extensions */
  virtual const char*
  getExtensions()=0;

  /**
   * Flags that tell you what capabilities this format supports.
   *
   * @return a bitmask of Flags
   */
  virtual int32_t
  getFlags()=0;

  /**
   * Find out if the given Flag is set for this ContainerFormat.
   */
  virtual bool
  getFlag(Flag flag) {
    return getFlags() & flag;
  }

  /**
   * Get total number of different codecs this container can output.
   */
  virtual int32_t
  getNumSupportedCodecs() = 0;
  /**
   * Get the Codec.ID for the n'th codec supported by this container.
   *
   * @param n The n'th codec supported by this codec. Lower n are higher priority.
   *   n must be < #getNumSupportedCodecs()
   * @return the Codec.ID at the n'th slot, or Codec.ID.CODEC_ID_NONE if none.
   */
  virtual Codec::ID
  getSupportedCodecId(int32_t n) = 0;
  /**
   * Get the 32-bit Codec Tag for the n'th codec supported by this container.
   *
   * @param n The n'th codec supported by this codec. Lower n are higher priority.
   *   n must be < #getNumSupportedCodecs()
   * @return the codec tag at the n'th slot, or 0 if none.
   */
  virtual uint32_t
  getSupportedCodecTag(int32_t n) = 0;

#ifndef SWIG
  /**
   * Get total number of different codecs this container can output.
   */
  static int32_t
  getNumSupportedCodecs(const struct AVCodecTag * const * tags);

  /**
   * Get the Codec.ID for the n'th codec supported by this container.
   *
   * @param n The n'th codec supported by this codec. Lower n are higher priority.
   *   n must be < #getNumSupportedCodecs()
   * @return the Codec.ID at the n'th slot, or Codec.ID.CODEC_ID_NONE if none.
   */
  static Codec::ID
  getSupportedCodecId(const struct AVCodecTag * const * tags, int32_t n);

  /**
   * Get the 32-bit Codec Tag for the n'th codec supported by this container.
   *
   * @param n The n'th codec supported by this codec. Lower n are higher priority.
   *   n must be < #getNumSupportedCodecs()
   * @return the codec tag at the n'th slot, or 0 if none.
   */
  static uint32_t
  getSupportedCodecTag(const struct AVCodecTag * const * tags, int32_t n);
#endif // !SWIG
  ContainerFormat();
  virtual
  ~ContainerFormat();
};

} /* namespace video */
} /* namespace humble */
} /* namespace io */
#endif /* CONTAINERFORMAT_H_ */
