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
 * Muxer.h
 *
 *  Created on: Aug 14, 2013
 *      Author: aclarke
 */

#ifndef MUXER_H_
#define MUXER_H_

#include <io/humble/video/Container.h>
#include <io/humble/video/Processor.h>
#include <io/humble/video/MuxerFormat.h>
#include <io/humble/video/KeyValueBag.h>
#include <io/humble/video/MuxerStream.h>
#include <io/humble/video/Encoder.h>

#ifndef SWIG
#ifdef MUXER_H_

#include <io/humble/video/customio/URLProtocolHandler.h>

#endif
#endif // ! SWIG

namespace io {
namespace humble {
namespace video {

/**
 * A Container that MediaPacket objects can be written to.
 */
class VS_API_HUMBLEVIDEO Muxer : public io::humble::video::Container,
  public virtual ProcessorEncodedSink
{
public:

  /**
   * Creates a new muxer.
   *
   * One of the three passed in parameter must be non-null. If the muxer requires a URL to write to,
   * then that must be specified.
   *
   * @param format If non null, this will be the format this muxer assumes it is writting packets in.
   * @param filename The filename/url to open. If format is null, this will also be examined to guess actual format.
   * @param formatName The formatname of the muxer to use. This will only be examined if format is null.
   *
   * @return a Muxer
   *
   * @throws InvalidArgument if all parameters are null.
   */
  static Muxer*
  make(const char* filename, MuxerFormat* format, const char* formatName);

  /**
   * Get the URL the Muxer was opened with.
   * May return null if unknown.
   * @return the URL opened, or null.
   */
  virtual const char*
  getURL();

  /**
   * Get the MuxerFormat associated with this Muxer
   * or null if unknown.
   */
  virtual MuxerFormat *
  getFormat() { return mFormat.get(); }

  /**
   * Muxers can only be in one of these states.
   */
  typedef enum State
  {
    /**
     * Initialized but not yet opened. Transitions to STATE_OPENED or STATE_ERROR.
     * New streams can be added.
     */
    STATE_INITED,
    /**
     * File is opened, and header is written. For most formats,
     * you can no longer add new streams. Check flags to find out if you can.
     */
    STATE_OPENED,

    /**
     * Trailer is written, file is closed and all file-resources have been released. The Muxer
     * should be discarded.
     */
    STATE_CLOSED,
    /**
     * An error has occured.
     */
    STATE_ERROR
  } State;

  /**
   * Get the current state of the Muxer.
   */
  virtual State
  getState() {
    return mState;
  }

  /**
   * Open the Muxer and write any headers.
   *
   * @param inputOptions muxer-specific options to set before opening the muxer. Can be null.
   * @param outputOptions if non null, the passed in bag will be emptied, and the filled
   *    with any options from inputOptions that could not be set on the muxer.
   */
  virtual void
  open(KeyValueBag* inputOptions, KeyValueBag* outputOptions);

  /**
   * Close the muxer and write any trailers.
   *
   * Note: Calls MUST call this method -- it will not automatically be called
   * when the object is finalized as some muxers struggle when you write trailers
   * on a different thread (the finalizer thread) than the header was written on.
   */
  virtual void
  close();

  /**
   * Get the number of streams in this container.
   */
  int32_t getNumStreams() { return getFormatCtx()->nb_streams; }

  /**
   * Set the buffer length Humble Video will suggest to FFMPEG for writing output data.
   *
   * If called when a Container is open, the call is ignored and -1 is returned.
   *
   * @param size The suggested buffer size.
   * @throws InvalidArgument if size <= 0
   */
  virtual void
  setOutputBufferLength(int32_t size);

  /**
   * Return the output buffer length.
   *
   * @return The input buffer length Humble Video told FFMPEG to assume.
   *   0 means FFMPEG should choose it's own size (and it'll probably be 32768).
   */
  virtual int32_t
  getOutputBufferLength();

  /**
   * Adds a new stream that will have packets written to it.
   *
   * Note on thread safety: Callers must ensure that the coder is not encoding or decoding
   * packets at the same time that Muxer#open or Muxer#close is being called.
   *
   * @param coder The coder that will be used for packets written to this stream.
   *
   * @throws InvalidArgument if encoder is null.
   * @throws InvalidArgument if encoder is not open.
   *
   */
  virtual MuxerStream*
  addNewStream(Coder* coder);

  /**
   * Get the MuxerStream at the given position.
   */
  virtual MuxerStream*
  getStream(int32_t position);

  /**
   * Writes the given packet to the Muxer.
   *
   * @param packet The packet to write.
   * @param forceInterleave If true, this Muxer will ensure that all packets are interleaved across streams
   *   (i.e. monotonically increasing timestamps in the Muxer container). If false, then the caller
   *   is responsible for ensuring the interleaving is valid for the container. Note this method is faster
   *   if forceInterleave is false.
   *
   * @returns true if all data has been flushed, false if data remains to be flushed.
   *
   * @throw InvalidArgument if packet is null.
   * @throw InvalidArgument if packet is not complete.
   * @throw RuntimeException for other errors.
   */
  virtual bool
  write(MediaPacket* packet, bool forceInterleave);

  virtual ProcessorResult sendPacket(MediaPacket *media);
  virtual ProcessorResult sendEncoded(MediaEncoded* media) {
    MediaPacket* m = dynamic_cast<MediaPacket*>(media);
    if (media && !m)
      throw io::humble::ferry::HumbleRuntimeError("expected a MediaPacket object");
    else
      return sendPacket(m);
  }

#if 0
#ifndef SWIG
  virtual int32_t acquire();
  virtual int32_t release();
#endif
#endif

protected:
  virtual AVFormatContext* getFormatCtx() { return mCtx; }
  /**
   * Function to log the write event as a trace
   */
  static void logWrite(Muxer* muxer, MediaPacket* in, MediaPacket* out, int32_t retval);
  /**
   * Log an open.
   */
  static void logOpen(Muxer*);

  /**
   * Takes the packet given (in whatever time base it was encoded with) and resets all time stamps
   * to align with the stream in this container that it will be added to.
   *
   * @param packet The packet to stamp. Packet#setStreamIndex must have been called to avoid an error,
   *  Packet#getStreamIndex must point to a stream number in this muxer.
   */
  static void stampOutputPacket(Container::Stream* stream, MediaPacket* packet);
  Muxer(MuxerFormat* format, const char* filename, const char* formatName);
  virtual
  ~Muxer();
private:
  State mState;
  AVFormatContext* mCtx;
  io::humble::video::customio::URLProtocolHandler* mIOHandler;

  io::humble::ferry::RefPointer<MuxerFormat> mFormat;
  int32_t mBufferLength;
};

} /* namespace video */
} /* namespace humble */
} /* namespace io */
#endif /* MUXER_H_ */
