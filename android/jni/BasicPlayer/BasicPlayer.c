/*
 * Main functions of BasicPlayer
 * 2011-2011 Jaebong Lee (novaever@gmail.com)
 *
 * BasicPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avcodec.h"
#include "avformat.h"
#include "swscale.h"
#include "BasicPlayer.h"
#include "BasicPlayerJni.h"


AVFormatContext *gFormatCtx = NULL;

AVCodecContext *gVideoCodecCtx = NULL;
AVCodecContext *gAudioCodecCtx = NULL;

AVCodec *gVideoCodec = NULL;
AVCodec *gAudioCodec = NULL;

int gVideoStreamIdx = -1;
int gAudioStreamIdx = -1;

AVFrame *gFrame = NULL;
AVFrame *gFrameRGB = NULL;

struct SwsContext *gImgConvertCtx = NULL;

int gPictureSize = 0;
uint8_t *gVideoBuffer = NULL;

char buff[1024];

void my_log(void* avcl, int level, const char *fmt, va_list vl) 
{
	snprintf(buff,  sizeof(buff), fmt, vl);
	PRINT(ANDROID_LOG_DEBUG, LOG_TAG, buff);
}

int openMovie(const char filePath[])
{
	int i;

	if (gFormatCtx != NULL) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "gFormatCtx != null");	
		return -1;
	}

	if (av_open_input_file(&gFormatCtx, filePath, NULL, 0, NULL) != 0) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "av_open_input_file failed!!!");	
		return -2;
	}

	if (av_find_stream_info(gFormatCtx) < 0) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "av_find_stream_info failed.");	
		return -3;
	}

	av_log_set_callback(my_log);



	dump_format(gFormatCtx, 0, filePath, 0);


	gVideoStreamIdx = -1;
	gAudioStreamIdx = -1;

	for (i = 0; i < gFormatCtx->nb_streams; i++) {
		if (gFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && gVideoStreamIdx < 0) {
			gVideoStreamIdx = i;
		}
		if (gFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && gAudioStreamIdx < 0) {
			gAudioStreamIdx = i;
		}
		
	}
	if (gVideoStreamIdx == -1) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "gVideoStreamIdx == -1");	
		return -4;
	}

	if (gAudioStreamIdx == -1) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "gAudioStreamIdx == -1");	
		return -4;
	}

	gVideoCodecCtx = gFormatCtx->streams[gVideoStreamIdx]->codec;


	gVideoCodec = avcodec_find_decoder(gVideoCodecCtx->codec_id);
	if (gVideoCodec == NULL) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "avcodec_find_decoder failed!!!");	
		return -5;
	}


	gAudioCodecCtx = gFormatCtx->streams[gAudioStreamIdx]->codec;
	gAudioCodec = avcodec_find_decoder(gAudioCodecCtx->codec_id);
	if (gAudioCodec == NULL) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "avcodec_find_decoder failed(audio)!!!");	
		return -5;
	}


	if (avcodec_open(gVideoCodecCtx, gVideoCodec) < 0) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "avcodec_open failed!!!");	
		return -6;
	}

	if (avcodec_open(gAudioCodecCtx, gAudioCodec) < 0) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "avcodec_open failed(audio)!!!");	
		return -6;
	}

	gFrame = avcodec_alloc_frame();
	if (gFrame == NULL) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "avcodec_alloc_frame failed!!!!");	
		return -7;
	}

	gFrameRGB = avcodec_alloc_frame();
	if (gFrameRGB == NULL) {
		LOG(ANDROID_LOG_DEBUG, LOG_TAG, "avcodec_alloc_frame failed!!!");	
		return -8;
	}

	gPictureSize = avpicture_get_size(PIX_FMT_RGB565LE, gVideoCodecCtx->width, gVideoCodecCtx->height);
	gVideoBuffer = (uint8_t*)(malloc(sizeof(uint8_t) * gPictureSize));

	avpicture_fill((AVPicture*)gFrameRGB, gVideoBuffer, PIX_FMT_RGB565LE, gVideoCodecCtx->width, gVideoCodecCtx->height);
	
	PRINT(ANDROID_LOG_DEBUG, LOG_TAG, "openMovie Success!!!! [%s]", filePath);	

	return 0;
}

int decodeFrame()
{
	int frameFinished = 0;
	AVPacket packet;
	
	while (av_read_frame(gFormatCtx, &packet) >= 0) {
		if (packet.stream_index == gVideoStreamIdx) {
			avcodec_decode_video2(gVideoCodecCtx, gFrame, &frameFinished, &packet);
			
			if (frameFinished) {
				gImgConvertCtx = sws_getCachedContext(gImgConvertCtx,
					gVideoCodecCtx->width, gVideoCodecCtx->height, gVideoCodecCtx->pix_fmt,
					gVideoCodecCtx->width, gVideoCodecCtx->height, PIX_FMT_RGB565LE, SWS_BICUBIC, NULL, NULL, NULL);
				
				sws_scale(gImgConvertCtx, gFrame->data, gFrame->linesize, 0, gVideoCodecCtx->height, gFrameRGB->data, gFrameRGB->linesize);
				
				av_free_packet(&packet);
		
				return 0;
			}
		}
		
		av_free_packet(&packet);
	}

	return -1;
}

void copyPixels(uint8_t *pixels)
{
	memcpy(pixels, gFrameRGB->data[0], gPictureSize);
}

int getWidth()
{
	return gVideoCodecCtx->width;
}

int getHeight()
{
	return gVideoCodecCtx->height;
}

void closeMovie()
{
	if (gVideoBuffer != NULL) {
		free(gVideoBuffer);
		gVideoBuffer = NULL;
	}
	
	if (gFrame != NULL)
		av_freep(gFrame);
	if (gFrameRGB != NULL)
		av_freep(gFrameRGB);

	if (gVideoCodecCtx != NULL) {
		avcodec_close(gVideoCodecCtx);
		gVideoCodecCtx = NULL;
	}

	if (gAudioCodecCtx != NULL) {
		avcodec_close(gAudioCodecCtx);
		gAudioCodecCtx = NULL;
	}
	
	if (gFormatCtx != NULL) {
		av_close_input_file(gFormatCtx);
		gFormatCtx = NULL;
	}
}
