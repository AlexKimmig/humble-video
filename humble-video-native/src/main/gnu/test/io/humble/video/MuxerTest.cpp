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
 * MuxerTest.cpp
 *
 *  Created on: Aug 14, 2013
 *      Author: aclarke
 */

#include "MuxerTest.h"
#include <io/humble/ferry/Logger.h>

#include <io/humble/video/Demuxer.h>
#include <io/humble/video/Decoder.h>
#include <io/humble/video/MediaPacket.h>
#include <io/humble/video/BitStreamFilter.h>

VS_LOG_SETUP(io.humble.video);

MuxerTest::MuxerTest() {

}

MuxerTest::~MuxerTest() {
}

void
MuxerTest::testCreation() {
  RefPointer<Muxer> muxer;

  muxer = Muxer::make("MuxerTest_testCreation.flv", 0, 0);

  RefPointer<MuxerStream> stream;

  RefPointer<Codec> c = Codec::findEncodingCodec(Codec::CODEC_ID_FLV1);
  RefPointer<Encoder> e = Encoder::make(c.value());
  RefPointer<Rational> t = Rational::make(1, 1000);

  e->setWidth(480);
  e->setHeight(320);
  e->setPixelFormat(PixelFormat::PIX_FMT_YUV420P);
  e->setTimeBase(t.value());
  e->setFlag(Coder::FLAG_GLOBAL_HEADER, true);

  e->open(0, 0);

  stream = muxer->addNewStream(e.value());

  muxer->open(0, 0);

  muxer->close();


}

void
MuxerTest::testRemuxing() {
//  TS_SKIP("leak fixed but still need to determine right muxing strategy");
  RefPointer<Muxer> muxer;

  muxer = Muxer::make("file:MuxerTest_testRemuxing.mp4", 0, 0);

  //  TestData::Fixture* fixture=mFixtures.getFixture("testfile_h264_mp4a_tmcd.mov");
  TestData::Fixture* fixture=mFixtures.getFixture("ucl_h264_aac.mp4");
  TS_ASSERT(fixture);
  char filepath[2048];
  mFixtures.fillPath(fixture, filepath, sizeof(filepath));

  RefPointer<Demuxer> demuxer = Demuxer::make();

  demuxer->open(filepath, 0, false, true, 0, 0);

  int32_t n = demuxer->getNumStreams();
  for(int i = 0; i < n; i++) {
    RefPointer<DemuxerStream> demuxerStream = demuxer->getStream(i);
    RefPointer<Decoder> d = demuxerStream->getDecoder();
    RefPointer<MuxerStream> muxerStream = muxer->addNewStream(d.value());
  }
  RefPointer<MediaPacket> packet = MediaPacket::make();

  muxer->open(0, 0);
  int32_t packetNo = 0;
  bool isMemcheck = getenv("VS_TEST_MEMCHECK");
  while(demuxer->read(packet.value()) >= 0) {
    muxer->write(packet.value(), false);
    ++packetNo;
    if (isMemcheck && packetNo > 10) {
      VS_LOG_DEBUG("Cutting short when running under valgrind");
      break;
    }
  }
  muxer->close();
  demuxer->close();
}
void
MuxerTest::testHLSRemuxing() {
//  TS_SKIP("leak fixed but still need to determine right muxing strategy");
  RefPointer<Muxer> muxer;

  muxer = Muxer::make("file:MuxerTest_testHLSRemuxing.m3u8", 0, "hls");
  muxer->setProperty("start_number", (int64_t)0LL);
  muxer->setProperty("hls_time", (int64_t)2LL);
  muxer->setProperty("hls_list_size", (int64_t)0LL);
  muxer->setProperty("hls_wrap", (int64_t)0LL);

  TestData::Fixture* fixture=mFixtures.getFixture("testfile_h264_mp4a_tmcd.mov");
//  TestData::Fixture* fixture=mFixtures.getFixture("ucl_h264_aac.mp4");
  TS_ASSERT(fixture);
  char filepath[2048];
  mFixtures.fillPath(fixture, filepath, sizeof(filepath));

  RefPointer<Demuxer> demuxer = Demuxer::make();

  demuxer->open(filepath, 0, false, true, 0, 0);

  int32_t n = demuxer->getNumStreams();
  for(int i = 0; i < n; i++) {
    RefPointer<DemuxerStream> demuxerStream = demuxer->getStream(i);
    RefPointer<Decoder> d = demuxerStream->getDecoder();
    if (d)
      RefPointer<MuxerStream> muxerStream = muxer->addNewStream(d.value());
  }
  RefPointer<MediaPacket> packet = MediaPacket::make();
  RefPointer<BitStreamFilter> vFilter = BitStreamFilter::make("h264_mp4toannexb");

  muxer->open(0, 0);

  // find the video stream
  int32_t numStreams = muxer->getNumStreams();
  for (int i = 0; i < numStreams; i++) {
    RefPointer<ContainerStream> stream = muxer->getStream(i);
    RefPointer<MediaParameters> p = stream->getMediaParameters();
    if (p->getType() == MediaDescriptor::MEDIA_VIDEO) {
      vFilter->setMediaParameters(p.value());
      vFilter->open(0, 0);
      break;
    }
  }
  int32_t packetNo = 0;
  bool isMemcheck = getenv("VS_TEST_MEMCHECK");
  while(demuxer->read(packet.value()) >= 0) {
    RefPointer<Coder> coder = packet->getCoder();
    if (!coder)
      // skip
      continue;

    if (coder->getCodecType() == MediaDescriptor::MEDIA_VIDEO) {
      // call the bit stream filter.
      TS_ASSERT_EQUALS(RESULT_SUCCESS, vFilter->send(packet.value()));
      TS_ASSERT_EQUALS(RESULT_SUCCESS, vFilter->receive(packet.value()));
    }
    muxer->write(packet.value(), false);
    ++packetNo;
    if (isMemcheck && packetNo > 10) {
      VS_LOG_DEBUG("Cutting short when running under valgrind");
      break;
    }
  }
  muxer->close();
  demuxer->close();
}
