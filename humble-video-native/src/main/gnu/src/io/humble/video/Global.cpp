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

#include <cmath>
#include <cstring>

#include <io/humble/ferry/JNIHelper.h>
#include <io/humble/ferry/Logger.h>
#include <io/humble/ferry/Mutex.h>


#include <io/humble/video/Global.h>
#include <io/humble/video/FfmpegIncludes.h>
#include <io/humble/video/VideoExceptions.h>

/**
 * WARNING: Do not use logging in this class, and do
 * not set any static file variables to values other
 * than zero.  The class loader
 * in Java will call Global::init, and MacOS will crash
 * if during the JNI library load the method we call calls
 * back into Java
 */
namespace io { namespace humble { namespace video
{
  using namespace io::humble::ferry;

  Global* Global::sGlobal = 0;
  /*
   * This function will be called back by Ffmpeg anytime
   * it wants to log.  We then use it to dump
   * stuff into our own logs.
   */
  static void
  humblevideo_log_callback(void* ptr, int level, const char* fmt, va_list va)
  {
    static Logger* ffmpegLogger = 0;
    AVClass* avc = ptr ? *(AVClass**)ptr : 0;

    int currentLevel = av_log_get_level();
//    fprintf(stderr, "current level: %d; log level: %d\n", level, currentLevel);
    if (level > currentLevel || currentLevel < AV_LOG_PANIC)
      // just return
      return;

    if (!ffmpegLogger)
    {
      Global::lock();
      if (!ffmpegLogger)
        ffmpegLogger = Logger::getStaticLogger( "org.ffmpeg" );
      Global::unlock();
    }

    Logger::Level logLevel;
    if (level <= AV_LOG_ERROR)
      logLevel = Logger::LEVEL_ERROR;
    else if (level <= AV_LOG_WARNING)
      logLevel = Logger::LEVEL_WARN;
    else if (level <= AV_LOG_INFO)
      logLevel = Logger::LEVEL_INFO;
    else if (level <= AV_LOG_DEBUG)
      logLevel = Logger::LEVEL_DEBUG;
    else
      logLevel = Logger::LEVEL_TRACE;

    // Revise the format string to add additional useful info
    char revisedFmt[1024];
    revisedFmt[sizeof(revisedFmt)-1] = 0;
    if (avc)
    {
      snprintf(revisedFmt, sizeof(revisedFmt), "[%s @ %p] %s",
          avc->item_name(ptr), ptr, fmt);
    }
    else
    {
      snprintf(revisedFmt, sizeof(revisedFmt), "%s", fmt);
    }
    int len = strlen(revisedFmt);
    if (len > 0 && revisedFmt[len-1] == '\n')
    {
      revisedFmt[len-1]=0;
      --len;
    }
    if (len > 0)
      // it's not useful to pass in filenames and line numbers here.
      ffmpegLogger->logVA(0, 0, logLevel, revisedFmt, va);
  }

  int
  Global :: avioInterruptCB(void*)
  {
    JNIHelper* helper = JNIHelper::getHelper();
    int retval = 0;
    if (helper) {
      retval = helper->isInterrupted();
    }
    return retval;
  }
  
  void
  Global :: init()
  {
    if (!sGlobal) {
      av_log_set_callback(humblevideo_log_callback);
      av_log_set_level (AV_LOG_ERROR); // Only log errors by default
      avformat_network_init();
      sGlobal = new Global();
    }
  }
  void
  Global :: deinit()
  {
    avformat_network_deinit();
  }
  void
  Global :: destroyStaticGlobal(JavaVM*vm,void*closure)
  {
    Global *val = (Global*)closure;
    if (!vm && val) {
      delete val;
    }
  }

  Global :: Global()
  {
    io::humble::ferry::JNIHelper::sRegisterTerminationCallback(
        Global::destroyStaticGlobal,
        this);
    mLock = io::humble::ferry::Mutex::make();
    mDefaultTimeBase = Rational::make(1, Global::DEFAULT_PTS_PER_SECOND);
  }

  Global :: ~Global()
  {
    if (mLock)
      mLock->release();
  }
  
  void
  Global :: lock()
  {
    Global* ctx = sGlobal;
    if (ctx && ctx->mLock)
      ctx->mLock->lock();
  }

  void
  Global :: unlock()
  {
    Global* ctx = sGlobal;
    if (ctx && ctx->mLock)
      ctx->mLock->unlock();
  }

  int32_t
  Global :: getVersionMajor()
  {
    Global::init();
    return VS_LIB_MAJOR_VERSION;
  }
  int32_t
  Global:: getVersionMinor()
  {
    Global::init();
    return VS_LIB_MINOR_VERSION;
  }
  
  const char *
  Global :: getVersionStr()
  {
    Global::init();
    return PACKAGE_VERSION;
  }
  int32_t
  Global :: getAVFormatVersion()
  {
    Global::init();
    return LIBAVFORMAT_VERSION_INT;
  }
  const char *
  Global :: getAVFormatVersionStr()
  {
    Global::init();
    return AV_STRINGIFY(LIBAVFORMAT_VERSION);
  }
  int
  Global :: getAVCodecVersion()
  {
    Global::init();
    return LIBAVCODEC_VERSION_INT;
  }
  const char *
  Global :: getAVCodecVersionStr()
  {
    Global::init();
    return AV_STRINGIFY(LIBAVCODEC_VERSION);
  }

  void
  Global :: setFFmpegLoggingLevel(int32_t level)
  {
    Global::init();
    if (level < AV_LOG_PANIC)
      av_log_set_level(AV_LOG_QUIET);
    else if (level < AV_LOG_FATAL)
      av_log_set_level(AV_LOG_PANIC);
    else if (level < AV_LOG_ERROR)
      av_log_set_level(AV_LOG_FATAL);
    else if (level < AV_LOG_WARNING)
      av_log_set_level(AV_LOG_ERROR);
    else if (level < AV_LOG_INFO)
      av_log_set_level(AV_LOG_WARNING);
    else if (level < AV_LOG_VERBOSE)
      av_log_set_level(AV_LOG_INFO);
    else if (level < AV_LOG_DEBUG)
      av_log_set_level(AV_LOG_VERBOSE);
    else
      av_log_set_level(AV_LOG_DEBUG);
//    fprintf(stderr, "FFmpeg logging level = %d\n", av_log_get_level());
  }

  Rational*
  Global::getDefaultTimeBase()
  {
    Global* ctx = sGlobal;

    return ctx ? ctx->mDefaultTimeBase.get() : 0;
  }

  void
  Global::catchException(const std::exception & e) {
    Logger* logger = Logger::getLogger("io.humble.video");
    try {
      JNIHelper* helper = JNIHelper::getHelper();
      if (!helper)
        throw e;

      JNIEnv* jenv = helper->getEnv();
      if (!jenv)
        throw e;

      if (dynamic_cast<const io::humble::video::PropertyNotFoundException *>(&e))
      {
        io::humble::ferry::JNIHelper::throwJavaException(jenv, "io/humble/video/PropertyNotFoundException", e);
      } else {
        JNIHelper::catchException(jenv, e);
      }
    } catch(std::exception & e1) {
      logger->error(__FILE__, __LINE__, "Got exception when handing exceptions. Yikes: %s", e1.what());
    } catch (...) {
      logger->error(__FILE__, __LINE__, "Got exception when handing exceptions. Yikes: %s", "totally unknown whack job error");
    }
    delete logger;
  }
}}}
