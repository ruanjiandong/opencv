/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"

#if defined(HAVE_FFMPEG)

#include <string>

#include "cap_ffmpeg_impl.hpp"

#define icvCreateFileCapture_FFMPEG_p cvCreateFileCapture_FFMPEG
#define icvReleaseCapture_FFMPEG_p cvReleaseCapture_FFMPEG
#define icvGrabFrame_FFMPEG_p cvGrabFrame_FFMPEG
#define icvRetrieveFrame_FFMPEG_p cvRetrieveFrame_FFMPEG
#define icvSetCaptureProperty_FFMPEG_p cvSetCaptureProperty_FFMPEG
#define icvGetCaptureProperty_FFMPEG_p cvGetCaptureProperty_FFMPEG
#define icvCreateVideoWriter_FFMPEG_p cvCreateVideoWriter_FFMPEG
#define icvReleaseVideoWriter_FFMPEG_p cvReleaseVideoWriter_FFMPEG
#define icvWriteFrame_FFMPEG_p cvWriteFrame_FFMPEG

namespace cv {
namespace {

class CvCapture_FFMPEG_proxy CV_FINAL : public cv::IVideoCapture
{
public:
    CvCapture_FFMPEG_proxy() { ffmpegCapture = 0; }
    CvCapture_FFMPEG_proxy(const cv::String& filename) { ffmpegCapture = 0; open(filename); }
    virtual ~CvCapture_FFMPEG_proxy() { close(); }

    virtual double getProperty(int propId) const CV_OVERRIDE
    {
        return ffmpegCapture ? icvGetCaptureProperty_FFMPEG_p(ffmpegCapture, propId) : 0;
    }
    virtual bool setProperty(int propId, double value) CV_OVERRIDE
    {
        return ffmpegCapture ? icvSetCaptureProperty_FFMPEG_p(ffmpegCapture, propId, value)!=0 : false;
    }
    virtual bool grabFrame() CV_OVERRIDE
    {
        return ffmpegCapture ? icvGrabFrame_FFMPEG_p(ffmpegCapture)!=0 : false;
    }
    virtual bool retrieveFrame(int, cv::OutputArray frame) CV_OVERRIDE
    {
        unsigned char* data = 0;
        int step=0, width=0, height=0, cn=0;

        if (!ffmpegCapture ||
           !icvRetrieveFrame_FFMPEG_p(ffmpegCapture, &data, &step, &width, &height, &cn))
            return false;
        cv::Mat(height, width, CV_MAKETYPE(CV_8U, cn), data, step).copyTo(frame);
        return true;
    }
    virtual bool open( const cv::String& filename )
    {
        close();

        ffmpegCapture = icvCreateFileCapture_FFMPEG_p( filename.c_str() );
        return ffmpegCapture != 0;
    }
    virtual void close()
    {
        if (ffmpegCapture
#if defined(HAVE_FFMPEG_WRAPPER)
                && icvReleaseCapture_FFMPEG_p
#endif
)
            icvReleaseCapture_FFMPEG_p( &ffmpegCapture );
        CV_Assert(ffmpegCapture == 0);
        ffmpegCapture = 0;
    }

    virtual bool isOpened() const CV_OVERRIDE { return ffmpegCapture != 0; }
    virtual int getCaptureDomain() CV_OVERRIDE { return CV_CAP_FFMPEG; }

protected:
    CvCapture_FFMPEG* ffmpegCapture;
};

} // namespace

cv::Ptr<cv::IVideoCapture> cvCreateFileCapture_FFMPEG_proxy(const cv::String& filename)
{
#if defined(HAVE_FFMPEG_WRAPPER)
    icvInitFFMPEG::Init();
    if (!icvCreateFileCapture_FFMPEG_p)
        return cv::Ptr<cv::IVideoCapture>();
#endif
    cv::Ptr<CvCapture_FFMPEG_proxy> capture = cv::makePtr<CvCapture_FFMPEG_proxy>(filename);
    if (capture && capture->isOpened())
        return capture;
    return cv::Ptr<cv::IVideoCapture>();
}

namespace {

class CvVideoWriter_FFMPEG_proxy CV_FINAL :
    public cv::IVideoWriter
{
public:
    CvVideoWriter_FFMPEG_proxy() { ffmpegWriter = 0; }
    CvVideoWriter_FFMPEG_proxy(const cv::String& filename, int fourcc, double fps, cv::Size frameSize, bool isColor) { ffmpegWriter = 0; open(filename, fourcc, fps, frameSize, isColor); }
    virtual ~CvVideoWriter_FFMPEG_proxy() { close(); }

    virtual void write(cv::InputArray image ) CV_OVERRIDE
    {
        if(!ffmpegWriter)
            return;
        CV_Assert(image.depth() == CV_8U);

        icvWriteFrame_FFMPEG_p(ffmpegWriter, (const uchar*)image.getMat().ptr(), (int)image.step(), image.cols(), image.rows(), image.channels(), 0);
    }
    virtual bool open( const cv::String& filename, int fourcc, double fps, cv::Size frameSize, bool isColor )
    {
        close();
        ffmpegWriter = icvCreateVideoWriter_FFMPEG_p( filename.c_str(), fourcc, fps, frameSize.width, frameSize.height, isColor );
        return ffmpegWriter != 0;
    }

    virtual void close()
    {
        if (ffmpegWriter
#if defined(HAVE_FFMPEG_WRAPPER)
                && icvReleaseVideoWriter_FFMPEG_p
#endif
        )
            icvReleaseVideoWriter_FFMPEG_p( &ffmpegWriter );
        CV_Assert(ffmpegWriter == 0);
        ffmpegWriter = 0;
    }

    virtual double getProperty(int) const CV_OVERRIDE { return 0; }
    virtual bool setProperty(int, double) CV_OVERRIDE { return false; }
    virtual bool isOpened() const CV_OVERRIDE { return ffmpegWriter != 0; }

protected:
    CvVideoWriter_FFMPEG* ffmpegWriter;
};

} // namespace

cv::Ptr<cv::IVideoWriter> cvCreateVideoWriter_FFMPEG_proxy(const cv::String& filename, int fourcc,
                                                           double fps, cv::Size frameSize, int isColor)
{
#if defined(HAVE_FFMPEG_WRAPPER)
    icvInitFFMPEG::Init();
    if (!icvCreateVideoWriter_FFMPEG_p)
        return cv::Ptr<cv::IVideoWriter>();
#endif
    cv::Ptr<CvVideoWriter_FFMPEG_proxy> writer = cv::makePtr<CvVideoWriter_FFMPEG_proxy>(filename, fourcc, fps, frameSize, isColor != 0);
    if (writer && writer->isOpened())
        return writer;
    return cv::Ptr<cv::IVideoWriter>();
}

} // namespace

#endif // defined(HAVE_FFMPEG)
