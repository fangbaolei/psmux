#include "Gb28181PsMux.h"
#include <string>
#include <stdio.h>
#include <string.h>

#define BUF_LEN (1024*1024)


typedef struct _ENC_NaluUnit
{
    int type;
    int size;
    char *data;
}ENC_NaluUnit;

int FindStartCode (char *Buf, int zeros_in_startcode)
{
    int info;
    int i;

    info = 1;
    for (i = 0; i < zeros_in_startcode; i++)
        if(Buf[i] != 0)
            info = 0;

    if(Buf[i] != 1)
        info = 0;
    return info;
}

int getNextNal(FILE* inpf, char* Buf)
{
    int pos = 0;
    int StartCodeFound = 0;
    int info2 = 0;
    int info3 = 0;

    while(!feof(inpf) && (Buf[pos++]=fgetc(inpf))==0);

    while (!StartCodeFound)
    {
        if (feof (inpf))
        {
            return pos-1;
        }
        Buf[pos++] = fgetc (inpf);
        info3 = FindStartCode(&Buf[pos-4], 3);
        if(info3 != 1)
            info2 = FindStartCode(&Buf[pos-3], 2);
        StartCodeFound = (info2 == 1 || info3 == 1);
    }
    fseek (inpf, -4, SEEK_CUR);
    return pos - 4;
}


class PsMuxContext
{
public:
    PsMuxContext();
    ~PsMuxContext();

    void ProcessVideo(guint8* buf, int len, FILE *fp);
    void ProcessAudio(guint8* buf, int len, FILE *fp);

private:
    Gb28181PsMux PsMux;
    StreamIdx vIdx;
    StreamIdx aIdx;
    guint8* pMuxBuf;
    gint64 pts;
    gint64 dts;
    FILE* fpps;
};


PsMuxContext::PsMuxContext()
{
    vIdx = PsMux.AddStream(PSMUX_ST_VIDEO_H264);
    aIdx = PsMux.AddStream(PSMUX_ST_VIDEO_MPEG4);
    pMuxBuf = new guint8[BUF_LEN];
}

PsMuxContext::~PsMuxContext()
{
    delete []pMuxBuf;
}

void PsMuxContext::ProcessVideo(guint8* buf, int len, FILE *fp)
{
    int MuxOutSize = 0;
    int ret = PsMux.MuxH264SingleFrame(buf, len, pts, dts, vIdx, pMuxBuf, &MuxOutSize, BUF_LEN);

    if (ret == 0 && MuxOutSize > 0){
        fwrite(pMuxBuf, MuxOutSize, 1, fp);
        fflush(fp);
    }

    unsigned char c = 0;
    if (!isH264Or265Frame(buf, &c)){
        return;
    }

    NAL_type Type = getH264NALtype(c);
    if ((Type == NAL_IDR) || (Type == NAL_PFRAME)){
        pts += 3600;
        dts += 3600;
    }
}

void PsMuxContext::ProcessAudio(guint8* buf, int len, FILE *fp)
{
    int MuxOutSize = 0;
    int ret = PsMux.MuxAudioFrame(buf, len, pts, dts, aIdx, pMuxBuf, &MuxOutSize, BUF_LEN);

    if (ret == 0 && MuxOutSize > 0){
        fwrite(pMuxBuf, MuxOutSize, 1, fp);
        fflush(fp);
    }

    pts += 160;
    dts += 160;
}


int main(int argc, char* argv[])
{
    char Buf[1024*40];
    char aBuf[1024];
    FILE* fp = fopen(argv[1], "rb");
    if (fp == NULL)
    {
        printf("can't open file %s\n", argv[1]);
        return -1;
    }

    FILE *fpps = fopen("test.ps", "wb+");
    if (fpps == NULL)
    {
        printf("can't open file =\n");
        return -1;
    }

    FILE *fpaac = fopen("song.aac", "rb");
    if (fpaac == NULL)
    {
        printf("can't open file =\n");
        return -1;
    }

    PsMuxContext psmuxcontext;
    int len;
    while(!feof(fp))
    {
        len = getNextNal(fp, Buf);
        int ret= fread(aBuf, 1, 160, fpaac);
        if(ret <= 0)
            break;

        psmuxcontext.ProcessVideo((guint8*)Buf, len, fpps);
        psmuxcontext.ProcessAudio((guint8*)aBuf, ret, fpps);
    }

   fclose(fp);
   fclose(fpps);

	return 0;
}

