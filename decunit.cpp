#include "global.h"
#include "decunit.h"

CDecUnit::CDecUnit()
{
	int i;
	m_bExitDec = false;

	for (i=0; i<MAX_NUM_FRAMES; i++) {
		m_pFrames[i] = new char [2*1920*1080];
		m_bIsEmptyFrame[i] = true;
	}
	for (i=0; i<3; i++) {
		m_jpg[i].p = NULL;
		m_jpg[i].len = 0;
	}

	m_nReadPos = 0;
	m_nWritePos = 0;

	m_i64TotalFrames = 0;

	m_pSampleBuff = new short [MAX_PCM_SAMPLES*2];
	m_nSamplesInBuff = 0;

	m_bEOF = false;
	m_bIsReady = false;
	m_bIsIndex = false;
	m_bIsStarted = false;

	pthread_mutex_init(&m_cs, NULL);
}

CDecUnit::~CDecUnit()
{
	int i;

	Terminate();

	delete [] m_pSampleBuff;

	for (i=0; i<MAX_NUM_FRAMES; i++) {
		delete [] m_pFrames[i];
	}
	for (i=0; i<3; i++) {
		if (m_jpg[i].p) {
			delete [] m_jpg[i].p;
		}
	}

	pthread_mutex_destroy(&m_cs);
}

bool CDecUnit::Open(char *filename, bool bIsIndex)
{
	m_bIsIndex = bIsIndex;
	sprintf(m_strFileName, filename);

	Start();

	return true;
}

void CDecUnit::Run()
{
	int i, ret, video_index;
	int frame_count = 0;

	AVFrame *pAF = NULL, *pVF = NULL;
	AVCodecContext *pADec = NULL, *pVDec = NULL;
	AVFormatContext *pIC = avformat_alloc_context();

	SwrContext *pAC = NULL;
	SwsContext *pVC = NULL;

#if 0
	if (m_bIsIndex == false) {
		_d("[DU] start decoding %s\n", m_strFileName);
	}
#endif
	ret = avformat_open_input(&pIC, m_strFileName, NULL, NULL);
	if (ret < 0) {
		_l("[DU] Open failed\n");
		goto exit_dec;
	}

	ret = avformat_find_stream_info(pIC, NULL);
	if (ret < 0) {
		_l("[DU] Couldn't find stream info\n");
		goto exit_dec;
	}

	for (i=0; i<(int)pIC->nb_streams; i++) {
		AVCodec *pc;

		pc = avcodec_find_decoder(pIC->streams[i]->codec->codec_id);
		if (!pc) {
			_l("[DU] Couldn't find decoder for stream %d\n", i);
			goto exit_dec;
		}

		ret = OpenCodec(pIC, pc->type, i);
		if (ret < 0) {
			_l("[DU] Couldn't open codec for stream %d\n", i);
			goto exit_dec;
		}

		if (pc->type == AVMEDIA_TYPE_AUDIO) {
			pADec = pIC->streams[i]->codec;
			pAF = av_frame_alloc();
		} else {
			pVDec = pIC->streams[i]->codec;
			video_index = i;
			pVF = av_frame_alloc();
		}
	}

	while(m_bExitDec == false)
    {
		AVPacket pkt;
		int got_frame;

		ret = av_read_frame(pIC, &pkt);
		if (ret < 0) {
			break;
		}

		if (pkt.stream_index == video_index)
        {
			avcodec_decode_video2(pVDec, pVF, &got_frame, &pkt);

			if (got_frame)
            {
				char *pFrame = m_pFrames[m_nWritePos];
                m_i64TotalFrames++;

				while(m_bIsEmptyFrame[m_nWritePos] == false)
                {
					if (m_bExitDec)
                    {
						break;
					}
					usleep(10);
				}

				if (m_bExitDec)
                {
					break;
				}

				pthread_mutex_lock(&m_cs);
				if (pVDec->pix_fmt != AV_PIX_FMT_UYVY422)
                {
					uint8_t *sd[4], *dd[4];
					int sl[4], dl[4];

					if (!pVC)
                    {
						pVC = sws_getContext(pVDec->width, pVDec->height, pVDec->pix_fmt, 
								1920, 1080, AV_PIX_FMT_UYVY422, SWS_BILINEAR, NULL, NULL, NULL);
					}

					sd[0] = pVF->data[0];
					sd[1] = pVF->data[1];
					sd[2] = pVF->data[2];
					sd[3] = pVF->data[3];

					sl[0] = pVF->linesize[0];
					sl[1] = pVF->linesize[1];
					sl[2] = pVF->linesize[2];
					sl[3] = pVF->linesize[3];

					dd[0] = (uint8_t*)pFrame;
					dd[1] = 0;
					dd[2] = 0;
					dd[3] = 0;

					dl[0] = 1920*2;
					dl[1] = 0;
					dl[2] = 0;
					dl[3] = 0;

					sws_scale(pVC, sd, sl, 0, pVDec->height, dd, dl);
				}
                else
                {
					memcpy(pFrame, pVF->data[0], 1920*1080*2);
				}

				m_bIsEmptyFrame[m_nWritePos] = false;
				m_nFrames[m_nWritePos] = (int)(m_i64TotalFrames - 1);
				m_nWritePos++;
				if (m_nWritePos >= MAX_NUM_FRAMES)
                {
						m_nWritePos = 0;
				}
				frame_count++;
				pthread_mutex_unlock(&m_cs);

				if (frame_count >= 3)
                {
					m_bIsReady = true;
				}
			}
		}
        else
        {
			if (m_bIsIndex)
            {
				continue;
			}

			avcodec_decode_audio4(pADec, pAF, &got_frame, &pkt);
			if (got_frame)
            {
				int nSamples = pAF->nb_samples;
				short *pPCM = &m_pSampleBuff[2*m_nSamplesInBuff];
				pthread_mutex_lock(&m_cs);

				if (pADec->sample_fmt != AV_SAMPLE_FMT_S16 || pAF->sample_rate != 48000)
                {
					if (!pAC)
                    {
						pAC = swr_alloc();
						swr_alloc_set_opts(pAC, 
								pAF->channel_layout, AV_SAMPLE_FMT_S16, 48000,
								pAF->channel_layout, pADec->sample_fmt, pAF->sample_rate, 0, NULL);
						swr_init(pAC);
					}
					nSamples = av_rescale_rnd(swr_get_delay(pAC, pAF->sample_rate) + pAF->nb_samples, 48000, pAF->sample_rate, AV_ROUND_UP);
					//_d("[DU] Add audio samples %d, %d (total %d)\n", pAF->nb_samples, nSamples, m_nSamplesInBuff);
					swr_convert(pAC, (uint8_t **)&pPCM, nSamples, (const uint8_t **)pAF->data, pAF->nb_samples);
				}
                else
                {
					memcpy(pPCM, pAF->data[0], pAF->nb_samples*2*2);
				}

				m_nSamplesInBuff += nSamples;
				pthread_mutex_unlock(&m_cs);

				while ((m_nSamplesInBuff + nSamples) >= MAX_PCM_SAMPLES)
                {
					if (m_bExitDec)
                    {
						break;
					}
					usleep(10);
				}

				if (m_bExitDec)
                {
					break;
				}
				//_d("[DU] Add audio samples %d, %d (total %d)\n", pAF->nb_samples, nSamples, m_nSamplesInBuff);
			}
		}
		av_free_packet(&pkt);
	}
	
	if (pVC) {
		sws_freeContext(pVC);
	}
	if (pAC) {
		swr_free(&pAC);
	}
	if (pAF) {
		av_frame_free(&pAF);
	}
	if (pVF) {
		av_frame_free(&pVF);
	}
	if (pADec) {
		avcodec_close(pADec);
	}
	if (pVDec) {
		avcodec_close(pVDec);
	}
	if (pIC) {
		avformat_close_input(&pIC);
	}

	m_bEOF = true;
	if (m_bIsIndex == false) {
#if 0
		time_t c = time(NULL) + (9*3600);
		struct tm *tp = gmtime(&c);
		_d("[DU] Stop playing at %02d:%02d:%02d (%04d-%02d-%02d)\n",
				tp->tm_hour, tp->tm_min, tp->tm_sec,
				tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday);
#endif
	}
	return;

exit_dec:
	_l("[DU] error occur...\n");
	m_bEOF = true;

	if (pVC) {
		sws_freeContext(pVC);
	}
	if (pAC) {
		swr_free(&pAC);
	}
	if (pAF) {
		av_frame_free(&pAF);
	}
	if (pVF) {
		av_frame_free(&pVF);
	}
	if (pADec) {
		avcodec_close(pADec);
	}
	if (pVDec) {
		avcodec_close(pVDec);
	}
	if (pIC) {
		avformat_close_input(&pIC);
	}
}

void CDecUnit::OnTerminate()
{
	m_bExitDec = true;
}

int CDecUnit::OpenCodec(AVFormatContext *pFmtCtx, AVMediaType nType, int nIndex)
{
	int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;

	ret = av_find_best_stream(pFmtCtx, nType, nIndex, -1, NULL, 0);
	if (ret < 0) {
        _l("[DU] Could not find stream\n");
        return ret;
    } else {
        st = pFmtCtx->streams[ret];

        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) 
		{
            _l("[DU] Failed to find a codec\n");
            return ret;
        }
		AVDictionary *opts = NULL;

		dec_ctx->workaround_bugs = 1;
		dec_ctx->lowres = 0;
		dec_ctx->idct_algo = FF_IDCT_AUTO;
		dec_ctx->skip_frame = AVDISCARD_DEFAULT;
		dec_ctx->skip_idct = AVDISCARD_DEFAULT;
		dec_ctx->skip_loop_filter = AVDISCARD_DEFAULT;
		dec_ctx->error_concealment = 3;
		if (dec->capabilities & CODEC_CAP_DR1) {
			dec_ctx->flags |= CODEC_FLAG_EMU_EDGE;
		}

		//av_dict_set(&opts, "threads", "auto", 0);
		av_dict_set(&opts, "threads", "1", 0);
        if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
            _l("[DU] Failed to open a codec\n");
            return ret;
        }
    }

    return 0;
}

char *CDecUnit::GetVideoFrame(int &nFrame, bool &bIsEOF)
{
	char *pFrame = NULL;
	if (m_bIsReady == false) {
		return NULL;
	}

	pthread_mutex_lock(&m_cs);
	if (m_bIsEmptyFrame[m_nReadPos] == false) {
		nFrame = m_nFrames[m_nReadPos];
		pFrame = m_pFrames[m_nReadPos];
	}

	pthread_mutex_unlock(&m_cs);

	bIsEOF = m_bEOF;

	return pFrame;
}

void CDecUnit::RetVideoFrame()
{
	if (m_bIsStarted == false) {
#if 0
		time_t c = time(NULL) + (9*3600);
		struct tm *tp = gmtime(&c);

		_d("[DU] Start playing at %02d:%02d:%02d (%04d-%02d-%02d)\n",
				tp->tm_hour, tp->tm_min, tp->tm_sec,
				tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday);
#endif	
		m_bIsStarted = true;
	}

	pthread_mutex_lock(&m_cs);
	m_bIsEmptyFrame[m_nReadPos] = true;
	m_nReadPos++;
	if (m_nReadPos >= MAX_NUM_FRAMES) {
		m_nReadPos = 0;
	}
	pthread_mutex_unlock(&m_cs);
}

bool CDecUnit::GetAudioSamples(short *pSamples, int nSamples)
{
	if (m_nSamplesInBuff < nSamples || m_bIsReady == false) {
		//_l("[DU] Insufficient audio samples, %d/%d\n", m_nSamplesInBuff, nSamples);
		return false;
	}

	memcpy(pSamples, m_pSampleBuff, nSamples*2*2);

	pthread_mutex_lock(&m_cs);
	memmove(m_pSampleBuff, &m_pSampleBuff[nSamples*2], 2*2*(m_nSamplesInBuff - nSamples));
	m_nSamplesInBuff -= nSamples;
	pthread_mutex_unlock(&m_cs);

	return true;
}

