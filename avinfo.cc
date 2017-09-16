extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
}
#include <iostream>


int main(int argc, char* argv[])
{
    char**  pp = argv+1;
    av_register_all();

#if 0
    AVFormatContext* fmt_ctx = avformat_alloc_context();
    while (*pp) 
    {
	const char*  filename = *pp++;

	avformat_open_input(&fmt_ctx, filename, NULL, NULL);
	avformat_find_stream_info(fmt_ctx,NULL);

	auto  duration = fmt_ctx->duration;
	avformat_close_input(&fmt_ctx);

	std::cout << filename << ": duration=" << duration << std::endl;
    }
    avformat_free_context(fmt_ctx);
#else
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag = NULL;
    AVCodecContext*  codec_ctx = NULL;

    int ret;
    av_register_all();

    while (*pp)
    {
	const char*  filename = *pp++;
	if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL))) {
	    std::cerr << "failed avformat_open_input - " << filename << std::endl;
	    continue;
	}

	std::cout << filename << std::endl;
	while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
	    std::cout << "  " << tag->key << "=" << tag->value << std::endl;
	}

	std::cout << "  duration=" << fmt_ctx->duration/AV_TIME_BASE << std::endl;

	avformat_find_stream_info(fmt_ctx, NULL);
	int  vidstream = -1;
	for (int i=0; i< fmt_ctx->nb_streams; i++) {
	    if (fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
		vidstream = i;
		break;
	    }
	}

	codec_ctx = fmt_ctx->streams[vidstream]->codec;
	std::cout << "  width=" << codec_ctx->width << std::endl
	          << " height=" << codec_ctx->height << std::endl;

	avformat_close_input(&fmt_ctx);
    }
#endif

    return 0;
}
