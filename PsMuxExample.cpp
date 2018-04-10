#include "Gb28181PsMux.h"
#include <string>
#include <stdio.h>
#include <string.h>

#define BUF_LEN (1024*1024)

struct PsMuxContext 
{
    PsMuxContext()
    {
        Idx = PsMux.AddStream(PSMUX_ST_VIDEO_H264);
        pMuxBuf = new guint8[BUF_LEN];
        pts = 0;
        dts = 0;
    }
    ~PsMuxContext()
    {
        delete []pMuxBuf;
    }
    virtual void Process(guint8* buf, int len)
    {
        int MuxOutSize = 0;
        int ret = PsMux.MuxH264SingleFrame(buf, len, pts, dts, Idx, pMuxBuf, &MuxOutSize, BUF_LEN);
        
        if (ret == 0 && MuxOutSize > 0){
            OnPsFrameOut(pMuxBuf, MuxOutSize, pts, dts);
        }
        else if(ret == 3){

        }
        else{

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

    void PsProcessSaveFile(std::string DstName)
    {
        fpps = fopen(DstName.c_str(), "wb+");
    }
    
    void OnPsFrameOut(guint8* buf, int len, gint64 pts, gint64 dts)
       {
           if (len > 0 && fpps)
           {
               fwrite(buf, len, 1, fpps);
               fflush(fpps);
           }
       }

private:
    Gb28181PsMux PsMux;
    StreamIdx Idx;
    guint8* pMuxBuf;
    gint64 pts;
    gint64 dts;
    FILE* fpps;

};


//遍历block拆分NALU,直到MaxSlice,不然一直遍历下去
int process_block(guint8* pBlock, int BlockLen, int MaxSlice,  PsMuxContext* PsDst)
{
    static guint8* pStaticBuf = new guint8[BUF_LEN];
    static int StaticBufSize = 0;

    int LastBlockLen = 0;

    memcpy(pStaticBuf+StaticBufSize, pBlock, BlockLen);

    LastBlockLen = StaticBufSize+BlockLen;

    guint8* pCurPos = pStaticBuf;

    guint8* NaluStartPos = NULL;


    //一段数据里最多NALU个数,这样SPS PPS 后的I帧那就不用遍历
    int iSliceNum = 0;

    while (LastBlockLen > 4)
    {
        if(isH264Or265Frame(pCurPos,NULL)){
            if (iSliceNum + 1 >= MaxSlice){//已经到达最大NALU个数,下面的不用找了把剩下的加上就是
                PsDst->Process(pCurPos, LastBlockLen);
                break;
            }

            if (NaluStartPos == NULL){
                NaluStartPos = pCurPos;
            }
            else{
                PsDst->Process(NaluStartPos, pCurPos-NaluStartPos);
                iSliceNum++;
                NaluStartPos = pCurPos;
            }
        }

        pCurPos++;
        LastBlockLen--;
    }

    //有剩下的,保存,和后面的拼起来
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

    guint8* fReadbuf = new guint8[BUF_LEN];
    struct PsMuxContext psmuxcontext;
    psmuxcontext.PsProcessSaveFile("test.ps");
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

        process_block(fReadbuf, fReadsz, 0xffff, &psmuxcontext);
    }

    delete []fReadbuf;

	return 0;
}

