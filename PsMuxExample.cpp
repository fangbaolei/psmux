#include "Gb28181PsMux.h"
#include <string>
#include <stdio.h>
#include <string.h>

#define BUF_LEN (1024*1024)

class PsMuxContext
{
public:
    PsMuxContext();
    ~PsMuxContext();

    void Process(guint8* buf, int len, FILE *fp);
    int  process_block(guint8* pBlock, int BlockLen, int MaxSlice, FILE *fp);

private:
    Gb28181PsMux PsMux;
    StreamIdx Idx;
    guint8* pMuxBuf;
    gint64 pts;
    gint64 dts;
    FILE* fpps;
};


PsMuxContext::PsMuxContext()
{
    Idx = PsMux.AddStream(PSMUX_ST_VIDEO_H264);
    pMuxBuf = new guint8[BUF_LEN];

}

PsMuxContext::~PsMuxContext()
{
    delete []pMuxBuf;
}

void PsMuxContext::Process(guint8* buf, int len, FILE *fp)
{
    int MuxOutSize = 0;
    int ret = PsMux.MuxH264SingleFrame(buf, len, pts, dts, Idx, pMuxBuf, &MuxOutSize, BUF_LEN);

    if (ret == 0 && MuxOutSize > 0){
        printf("MuxOutSize %d\n", MuxOutSize);
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

//����block���NALU,ֱ��MaxSlice,��Ȼһֱ������ȥ
int PsMuxContext::process_block(guint8* pBlock, int BlockLen, int MaxSlice, FILE *fp)
{
    static guint8* pStaticBuf = new guint8[BUF_LEN];
    static int StaticBufSize = 0;

    int LastBlockLen = 0;

    memcpy(pStaticBuf+StaticBufSize, pBlock, BlockLen);

    LastBlockLen = StaticBufSize+BlockLen;

    guint8* pCurPos = pStaticBuf;

    guint8* NaluStartPos = NULL;


    //һ�����������NALU����,����SPS PPS ���I֡�ǾͲ��ñ���
    int iSliceNum = 0;

    while (LastBlockLen > 4)
    {
        if(isH264Or265Frame(pCurPos,NULL)){
            if (iSliceNum + 1 >= MaxSlice){//�Ѿ��������NALU����,����Ĳ������˰�ʣ�µļ��Ͼ���
                this->Process(pCurPos, LastBlockLen, fp);
                break;
            }

            if (NaluStartPos == NULL){
                NaluStartPos = pCurPos;
            }
            else{
                this->Process(NaluStartPos, pCurPos-NaluStartPos, fp);
                iSliceNum++;
                NaluStartPos = pCurPos;
            }
        }

        pCurPos++;
        LastBlockLen--;
    }

    //��ʣ�µ�,����,�ͺ����ƴ����
    if (NaluStartPos){
        memcpy(pStaticBuf, NaluStartPos, LastBlockLen);
        StaticBufSize = LastBlockLen;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int Circle = 0;

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

    guint8* fReadbuf = new guint8[BUF_LEN];
    PsMuxContext psmuxcontext;
    while(1) {
        int fReadsz = fread(fReadbuf, 1, BUF_LEN, fp);

        if(fReadsz <= 0){

            if (Circle){
                fseek(fp, 0, SEEK_SET);
                continue;
            }
            else{
                break;
            }
        }

        psmuxcontext.process_block(fReadbuf, fReadsz, 0xffff, fpps);
    }

    delete []fReadbuf;

	return 0;
}

