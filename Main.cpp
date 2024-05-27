//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm* MainForm;

//---------------------------------------------------------------------------
#include <bass.h>

#include <System.Math.hpp> // for RoundTo()
#include <malloc.h> // for alloca(), very important

// for bass library ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int device = -1;
int frequency = 44100;
int errInitBass = 0;

HSYNC hdlSync;
HSTREAM hdlStream;
HPLUGIN mPlug; // for plugins
UnicodeString arrLibs[8] = {
    L"bassflac.dll",
    L"bassape.dll",
    L"bassopus.dll",
    L"basswebm.dll",
    L"basswma.dll",
    L"bassalac.dll",
    L"bass_ac3.dll",
    L"bass_aac.dll",
};

// for audio wave ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include <Vcl.GraphUtil.hpp> //
#include <Vcl.Graphics.hpp> // for tbitmap
Graphics::TBitmap* bitLevel;
Graphics::TBitmap* bitSpectrum;

DynamicArray<Cardinal> Data;

UnicodeString caption = L"";
UnicodeString capMOD = L"%s    ❤    %s %s %s";
UnicodeString capWIN = L"Wave  -  Windows GDI";
UnicodeString capRAD = L"Wave  -  C++ Builder API";

UnicodeString fileName;
UnicodeString fileType = L"*.mp3";
UnicodeString wrkPath = L"R:\\Downloads";
UnicodeString appPath, plgPath = L"Plugins";

Cardinal mp3Len;
Cardinal posEnd;
Cardinal posStart;

double pos;
double len;
UnicodeString mTime;

bool flgPlay = false;
double nsScale = 1.0;

int nCl = 0;

#define flgMemo 0
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// flgMemo 0 == false, hide memo for normal running
// flgMemo 1 ==  ture, show memo for testing

#define flgColor 0
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// flgColor 0 == false, play the music
// flgColor 1 ==  ture, draw the color bar

#define flgStereo 1
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// flgStereo 0 == false, draw one channel for peak spectrum, (left + right) /2
// flgStereo 1 ==  ture, draw two channel for peak spectrum

#define flgBuilder 1
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// flgBuilder 0 == false, drawing is using windows gdi
// flgBuilder 1 ==  ture, drawing is using c++ builder api

// specMode = 0, "normal" FFT
// specMode = 1, logarithmic, combine bins, fast
// specMode = 2, logarithmic, combine bins, slowly falling
// specMode = 3, "3d"
// specMode = 4, waveform
// specMode = 5, momo solid waveform
int specMode = 1; // spectrum mode
int specPos = 0; // marker pos for 3d mode
float* buf;
BASS_CHANNELINFO ci;
int nY;
int v, y;
int nW; // peak level bar width

int fftPeak[128];
int fftFall[128];

float fft[1024];

float peak;
int b0;
int b1;
int nPcl;

#if flgColor
const int BANDS = 64;
#else
const int BANDS = 128;
#endif

int aPeak[1024] = { 0 };
int bPeak[1024] = { 0 };

#if flgBuilder

TRect mRct;

int nPal[256][3];

#else // windows gdi

int SPECWIDTH;
int SPECHEIGHT;
RGBQUAD* pal;
HWND win;
HDC dc;
HDC specDc;
HBITMAP specBmp;
BYTE* specBuf;
BITMAPINFOHEADER* bh;

int dP[8912] = { 0 };

#endif

//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner) : TForm(Owner) {}
//---------------------------------------------------------------------------

__fastcall TMainForm::~TMainForm(void)
{
    initial();

    delete bitLevel;

#if flgBuilder
    delete bitSpectrum;
#else
    if (specDc)
        DeleteDC(specDc);
    if (specBmp)
        DeleteObject(specBmp);
#endif
    TmrColor->Enabled = false;

    BASS_ChannelRemoveSync(hdlStream, hdlSync);
    BASS_ChannelStop(hdlStream);
    BASS_ChannelFree(hdlStream);
    BASS_StreamFree(hdlStream);
    BASS_PluginFree(0);
    BASS_Stop();
    BASS_Free();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormShow(TObject* Sender)
{
    UnicodeString msgInfo;
    switch (errInitBass) {
        case 1:
            msgInfo = L"Hello,";
            msgInfo += L"\n\n  bass.dll and bass.lib version are different.";
            msgInfo += L"  \n  Must be the same v2.4 and up.\n";
            MessageDlg(msgInfo, mtError, TMsgDlgButtons() << mbAbort, 0, mbAbort);
            Application->Terminate();
            break;
        case 2:
            msgInfo = L"Hello,";
            msgInfo += L"\n\n  bass.dll cannot initialize audio output device.\n";
            MessageDlg(msgInfo, mtError, TMsgDlgButtons() << mbCancel, 0, mbCancel);
            Application->Terminate();
            break;
        default:
            break;
    }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormResize(TObject* Sender)
{
    BtnOpen->Left = MainForm->Width - 266 * nsScale;
    BtnPlay->Left = MainForm->Width - 185 * nsScale;
    BtnRemove->Left = MainForm->Width - 104 * nsScale;
}

//---------------------------------------------------------------------------

void __fastcall TMainForm::FormCreate(TObject* Sender)
{
    // check the bass.dll and bass.lib version
    if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
        errInitBass = 1;
    }

    // free before initial device
    BASS_Free();

    // initialize output device
    if (!BASS_Init(device, frequency, 0, 0, NULL)) {
        errInitBass = 2;
    }

    // set loop for playing music
    if (ChkLoop->Checked) {
        BASS_ChannelFlags(hdlStream, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    }
    else {
        BASS_ChannelFlags(hdlStream, 0, BASS_SAMPLE_LOOP);
    }

    // get the program path
    appPath = ExtractFilePath(Application->ExeName);
    if (!DirectoryExists(wrkPath)) {
        wrkPath = appPath;
    }

    plgPath = Format(L"%s\%s", ARRAYOFCONST((appPath, plgPath)));
    for (int k = 0; k < std::size(arrLibs); k++) {
        mPlug = BASS_PluginLoad(Format(L"%s\\%s", ARRAYOFCONST((plgPath, arrLibs[k]))).c_str(), 0);
        if (mPlug > 0) {
            const BASS_PLUGININFO* info = BASS_PluginGetInfo(mPlug); // get the plugin info
            for (int a = 0; a < info->formatc; a++) { // display the array of formats...
                fileType += Format(L";%s", ARRAYOFCONST((info->formats[a].exts)));
                // Memo->Lines->Add(arrLibs[k]);
                // Memo->Lines->Add(fileType);
                // Memo->Lines->Add(Format(L"ctype=%x; name=%s; exts=%s", ARRAYOFCONST((info->formats[a].ctype, info->formats[a].name, info->formats[a].exts))));
            }
        }
    }

    // get the screen scale factor
    HWND activeWindow = GetActiveWindow();
    HMONITOR monitor = MonitorFromWindow(activeWindow, MONITOR_DEFAULTTONEAREST);
    UINT iX, iY;
    if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &iX, &iY)) && (iX > 0) && (iY > 0)) {
        nsScale = 1.0 * iX / USER_DEFAULT_SCREEN_DPI; // 1.25
        //nsScale = MulDiv(100, x, USER_DEFAULT_SCREEN_DPI);  // 125
    }

    bitLevel = new Graphics::TBitmap();
    bitSpectrum = new Graphics::TBitmap();

    bitLevel->Width = PaintBoxLevel->Width;
    bitLevel->Height = PaintBoxLevel->Height;
    bitLevel->Canvas->Brush->Color = clBlack;
    bitLevel->Canvas->FillRect(Rect(0, 0, bitLevel->Width, bitLevel->Height));
    PaintBoxLevel->Repaint();

    bitSpectrum->Width = PaintBoxSpectrum->Width;
    bitSpectrum->Height = PaintBoxSpectrum->Height;
    bitSpectrum->Canvas->Brush->Color = clBlack;
    bitSpectrum->Canvas->FillRect(Rect(0, 0, bitSpectrum->Width, bitSpectrum->Height));
    PaintBoxSpectrum->Repaint();

    nW = floor(PaintBoxSpectrum->Width / BANDS);

#if flgColor || flgMemo
    Memo->Visible = true;
#else
    Memo->Visible = false;
#endif

#if flgBuilder

    caption = capRAD;

    // peak level color
    for (int i = 0; i < 128; i++) {
        nPal[i][0] = 2 * i; // red
        nPal[i][1] = 154; // green
        nPal[i][2] = 255; // blue
    }
    // 3d spectrum color
    for (int i = 0; i < 32; i++) {
        nPal[128 + 00 + i][0] = 0;
        nPal[128 + 00 + i][1] = 0;
        nPal[128 + 00 + i][2] = 8 * i;

        nPal[128 + 32 + i][0] = 8 * i;
        nPal[128 + 32 + i][1] = 0;
        nPal[128 + 32 + i][2] = 255;

        nPal[128 + 64 + i][0] = 255;
        nPal[128 + 64 + i][1] = 8 * i;
        nPal[128 + 64 + i][2] = 8 * (31 - i);

        nPal[128 + 96 + i][0] = 255;
        nPal[128 + 96 + i][1] = 255;
        nPal[128 + 96 + i][2] = 8 * i;
    }

#else

    caption = capWIN;

    SPECWIDTH = PaintBoxSpectrum->Width + 1;
    SPECHEIGHT = PaintBoxSpectrum->Height;

    // create bitmap to draw spectrum in (8 bit for easy updating)
    BYTE data[2000] = { 0 };
    //BITMAPINFOHEADER* bh = (BITMAPINFOHEADER*)data;
    bh = (BITMAPINFOHEADER*)data;
    //RGBQUAD* pal;
    pal = (RGBQUAD*)(data + sizeof(*bh));
    int a;
    bh->biSize = sizeof(*bh);
    bh->biWidth = SPECWIDTH;
    bh->biHeight = SPECHEIGHT; // upside down (line 0=bottom)
    bh->biPlanes = 1;
    bh->biBitCount = 8;
    bh->biClrUsed = bh->biClrImportant = 256;

    // setup palette
    // peak level color
    for (int a = 1; a < 128; a++) {
        pal[a].rgbRed = 2 * a;
        pal[a].rgbGreen = 154;
        pal[a].rgbBlue = 255;
    }
    // 3d spectrum color
    for (int a = 0; a < 32; a++) {
        pal[128 + 00 + a].rgbBlue = 8 * a;
        pal[128 + 32 + a].rgbBlue = 255;
        pal[128 + 32 + a].rgbRed = 8 * a;
        pal[128 + 64 + a].rgbRed = 255;
        pal[128 + 64 + a].rgbBlue = 8 * (31 - a);
        pal[128 + 64 + a].rgbGreen = 8 * a;
        pal[128 + 96 + a].rgbRed = 255;
        pal[128 + 96 + a].rgbGreen = 255;
        pal[128 + 96 + a].rgbBlue = 8 * a;
    }
    // create the bitmap
    specBmp = CreateDIBSection(0, (BITMAPINFO*)bh, DIB_RGB_COLORS, (void**)&specBuf, NULL, 0);
    specDc = CreateCompatibleDC(0);
    SelectObject(specDc, specBmp);

#endif

    MainForm->Caption = caption;

    LabelLeft->Width = 0;
    LabelRight->Width = 0;
    LabelLeft->Left = 0;
    LabelRight->Left = 0;
    LabelLeft->Top = ShapeLeft->Top + 3;
    LabelRight->Top = ShapeRight->Top + 1;
    LabelLeft->Color = TColor(RGB(119, 221, 119));
    LabelRight->Color = TColor(RGB(255, 145, 164));
    LabelLeft->Caption = L"";
    LabelRight->Caption = L"";

    ShapePos->Pen->Style = psClear;
    ShapePos->Brush->Color = clWhite;
    ShapePos->Width = 3;
    ShapePos->Height = PaintBoxLevel->Height;
    ShapePos->Top = 0;
    ShapePos->Left = -1;

    ShapeStart->Pen->Style = psClear;
    ShapeStart->Brush->Color = clYellow;
    ShapeStart->Width = 3;
    ShapeStart->Height = PaintBoxLevel->Height;
    ShapeStart->Top = 0;
    ShapeStart->Visible = false;

    ShapeEnd->Pen->Style = psClear;
    ShapeEnd->Brush->Color = clRed;
    ShapeEnd->Width = 3;
    ShapeEnd->Height = PaintBoxLevel->Height;
    ShapeEnd->Top = 0;
    ShapeEnd->Visible = false;

    if (!DirectoryExists(wrkPath)) {
        wrkPath = ExtractFilePath(Application->ExeName);
    }

    // fileName = L"R:\\Downloads\\Wave\\start.mp3";
    fileName = Format(L"%sstart.mp3", ARRAYOFCONST((appPath)));
    OpenDialog->Title = L"Please select an audio file...";
    OpenDialog->Filter = Format(L"Audio files|%s", ARRAYOFCONST((fileType)));
    OpenDialog->FilterIndex = 0;
    OpenDialog->InitialDir = wrkPath;
    if (!FileExists(fileName)) {
        BtnOpen->Click();
    }
    else {
        openFile();
    }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::initial(void)
{
    Data.Length = 0;
    ShapePos->Left = -1;
    flgPlay = false;
    TmrPos->Enabled = false;
    TmrColor->Enabled = false;
    ShapeStart->Visible = false;
    ShapeEnd->Visible = false;
    BtnPlay->Caption = L"&Play";
    LabelLeft->Width = 0;
    LabelRight->Width = 0;
    BASS_ChannelRemoveSync(hdlStream, hdlSync);
    BASS_ChannelStop(hdlStream);
    BASS_ChannelFree(hdlStream);
    BASS_StreamFree(hdlStream);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::getPal(RGBQUAD &p)
{
    int rr = (unsigned int)(p.rgbRed);
    int gg = (unsigned int)(p.rgbGreen);
    int bb = (unsigned int)(p.rgbBlue);
    Memo->Lines->Add(Format(L"%s --> RGB(%d, %d, %d)", ARRAYOFCONST((Format(L"%3d", ARRAYOFCONST((nCl))), rr, gg, bb))));
}
// ---------------------------------------------------------------------------
// convert seconds -- > hh:mm:ss.ms or mm:ss.ms
UnicodeString __fastcall TMainForm::secToMark(double nSec)
{
    double hh = 0, mm, ss;
    UnicodeString cTime;

    ss = fmod(nSec, 60);
    mm = Int(nSec / 60);
    if (mm >= 60) {
        hh = Int(mm / 60);
        mm = fmod(mm, 60);
    }

    cTime = Format(L"%s:%s", ARRAYOFCONST((FormatFloat(L"00", mm), FormatFloat(L"00.00", ss))));
    if (hh > 0)
        cTime = Format(L"%s:%s", ARRAYOFCONST((FormatFloat(L"00", hh), cTime)));

    return cTime;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::drawPeak(void)
{
    int nCh;
    SmallInt nL, nR;

    bitLevel->Width = Data.Length;
    bitLevel->Height = PaintBoxLevel->Height;
    nCh = bitLevel->Height / 2;

    // background color
    bitLevel->Canvas->Brush->Color = clBlack;

    // color for audio wave
    //bitLevel->Canvas->Pen->Color = clLime;
    bitLevel->Canvas->Pen->Color = TColor(0xff9a32);
    bitLevel->Canvas->FillRect(Bounds(0, 0, bitLevel->Width, bitLevel->Height));

    for (int i = 0; i < Data.Length; i++) {
        nL = LOWORD(Data[i]);
        nR = HIWORD(Data[i]);

#if flgStereo
        // stereo, two channel
        int L1 = nCh - Int((double)nL / (double)32768 * nCh);
        int R1 = nCh + Int((double)nR / (double)32768 * nCh);
        bitLevel->Canvas->MoveTo(i, L1);
        bitLevel->Canvas->LineTo(i, R1);
#else
        // combine to one channel
        int L1 = bitLevel->Height - Int((double)(nL + nR) / 2 / (double)32767 * bitLevel->Height);
        bitLevel->Canvas->MoveTo(i, L1);
        bitLevel->Canvas->LineTo(i, bitLevel->Height);
#endif
    }

    PaintBoxLevel->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::openFile(void)
{
    initial();

    bitLevel->Canvas->Brush->Color = clBlack;
    bitLevel->Canvas->FillRect(Rect(0, 0, bitLevel->Width, bitLevel->Height));
    PaintBoxLevel->Repaint();

    bitSpectrum->Canvas->Brush->Color = clBlack;
    bitSpectrum->Canvas->FillRect(Rect(0, 0, bitSpectrum->Width, bitSpectrum->Height));
    PaintBoxSpectrum->Repaint();

    hdlStream = BASS_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, BASS_UNICODE);

    if (hdlStream < BASS_ERROR_ENDED) {
        LabelHint->Caption = Format(L"Cannot open: %s", ARRAYOFCONST((ExtractFileName(fileName))));
        return;
    }

    mp3Len = BASS_ChannelGetLength(hdlStream, BASS_POS_BYTE);
    // When using BASS_ChannelGetLevel to obtain the peak value
    // it is in units of 20ms; first obtain the total time
    len = BASS_ChannelBytes2Seconds(hdlStream, BASS_ChannelGetLength(hdlStream, BASS_POS_BYTE));
    mTime = secToMark(len);

    MainForm->Caption = Format(capMOD, ARRAYOFCONST((caption, mTime, L"", L"")));
    LabelHint->Caption = Format(L"Audio: %s", ARRAYOFCONST((ExtractFileName(fileName))));

    // len * 1000 / 20 + 1 is the total peak data that can be obtained
    // and is also the required size of the array
    Data.Length = Int(len * 50 + 1);

    // Re-establish the file stream wavStream
    // the last parameter is: BASS_STREAM_DECODE
    // so that the waveform data can be read in advance
    HSTREAM wavStream;
    wavStream = BASS_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, BASS_STREAM_DECODE);

    // Loop through the peak data to fill the array
    Cardinal i;
    for (int i = 0; i < Data.Length; i++) {
        Data[i] = BASS_ChannelGetLevel(wavStream);
    }

    BASS_StreamFree(wavStream);
    drawPeak();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BtnOpenClick(TObject* Sender)
{
    if (OpenDialog->Execute()) {
        fileName = OpenDialog->FileName;
        openFile();
    }
    else {
        // MessageDlg(L"cannot open mp3", mtError, TMsgDlgButtons() << mbClose, 0, mbClose);
        // Application->Terminate();
        return;
    }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BtnPlayClick(TObject* Sender)
{
#if flgColor
    if (flgPlay) {
        flgPlay = false;
        BtnPlay->Caption = L"&Play";
        TmrColor->Enabled = false;
        Memo->Lines->Add(L"pause");
    }
    else {
        flgPlay = true;
        BtnPlay->Caption = L"&Pause";
        TmrColor->Enabled = true;
        Memo->Lines->Add(L"start");
    }
#else
    if (flgPlay) {
        flgPlay = false;
        BtnPlay->Caption = L"&Play";
        TmrPos->Enabled = false;
        BASS_ChannelStop(hdlStream);
    }
    else {
        flgPlay = true;
        BtnPlay->Caption = L"&Pause";
        TmrPos->Enabled = true;
        BASS_ChannelPlay(hdlStream, FALSE);
    }
#endif
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::BtnRemoveClick(TObject* Sender)
{
    ShapeStart->Visible = false;
    ShapeEnd->Visible = false;
    BASS_ChannelRemoveSync(hdlStream, hdlSync);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::ChkLoopClick(TObject* Sender)
{
    if (ChkLoop->Checked) {
        BASS_ChannelFlags(hdlStream, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    }
    else {
        BASS_ChannelFlags(hdlStream, 0, BASS_SAMPLE_LOOP);
    }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TmrPosTimer(TObject* Sender)
{
    if (BASS_ChannelIsActive(hdlStream) == BASS_ACTIVE_PLAYING) {
        pos = BASS_ChannelBytes2Seconds(hdlStream, BASS_ChannelGetPosition(hdlStream, BASS_POS_BYTE));
        MainForm->Caption = Format(capMOD, ARRAYOFCONST((caption, secToMark(pos), L"▹", mTime)));

        ShapePos->Left = Int((double)BASS_ChannelGetPosition(hdlStream, BASS_POS_BYTE) / mp3Len * PaintBoxLevel->Width);

        // channel peak level
        Cardinal Level = BASS_ChannelGetLevel(hdlStream);
        // The lower 16 bits are the peak value of the left channel
        // the upper 16 bits are the peak value of the right channel
        LabelLeft->Width = Int((double)LOWORD(Level) / 32768 * PaintBoxSpectrum->Width);
        LabelRight->Width = Int((double)HIWORD(Level) / 32768 * PaintBoxSpectrum->Width);

#if flgBuilder

        // mono solid waveform
        if (specMode == 5) {
            bitSpectrum->Canvas->Brush->Color = clBlack;
            bitSpectrum->Canvas->FillRect(Rect(0, 0, bitSpectrum->Width, bitSpectrum->Height));

            buf = (float*)alloca(bitSpectrum->Width * sizeof(float)); // allocate buffer for data
            BASS_ChannelGetData(hdlStream, buf, bitSpectrum->Width * sizeof(float) | BASS_DATA_FLOAT); // get the sample data

            for (int x = 0; x < bitSpectrum->Width; x++) {
                v = (1 - buf[x]) * bitSpectrum->Height / 2; // invert and scale to fit display
                if (v < 0)
                    v = 0;
                else if (v >= bitSpectrum->Height)
                    v = bitSpectrum->Height - 1;
                if (!x)
                    y = v;
                do { // draw line from previous sample...
                    if (y < v)
                        y++;
                    else if (y > v)
                        y--;
                    bitSpectrum->Canvas->Pixels[x][y] = TColor(RGB(0, 154, 255));
                    //specBuf[y * SPECWIDTH + x] = 1; // left=green, right=red (could add more colours to palette for more chans)
                } while (y != v);
            }
        }
        // waveform
        else if (specMode == 4) {
            bitSpectrum->Canvas->Brush->Color = clBlack;
            bitSpectrum->Canvas->FillRect(Rect(0, 0, bitSpectrum->Width, bitSpectrum->Height));

            BASS_ChannelGetInfo(hdlStream, &ci); // get number of channels
            buf = (float*)alloca(ci.chans * bitSpectrum->Width * sizeof(float)); // allocate buffer for data
            BASS_ChannelGetData(hdlStream, buf, ci.chans * bitSpectrum->Width * sizeof(float) | BASS_DATA_FLOAT); // get the sample data

            for (int c = 0; c < ci.chans; c++) {
                for (int x = 0; x < bitSpectrum->Width; x++) {
                    v = (1 - buf[x * ci.chans + c]) * bitSpectrum->Height / 2; // invert and scale to fit display

                    if (ci.chans > 1) {
                        v = (c & 1) ? v - 3 : v + 3;
                    }

                    if (v < 0)
                        v = 0;
                    else if (v >= bitSpectrum->Height)
                        v = bitSpectrum->Height - 1;
                    if (!x)
                        y = v;
                    do { // draw line from previous sample...
                        if (y < v)
                            y++;
                        else if (y > v)
                            y--;
                        if (ci.chans > 1) {
                            bitSpectrum->Canvas->Pixels[x][y] = (c & 1) ? TColor(RGB(119, 221, 119)) : TColor(RGB(255, 145, 164));
                        }
                        else {
                            bitSpectrum->Canvas->Pixels[x][y] = TColor(RGB(0, 154, 255));
                        }
                    } while (y != v);
                }
            }
        }
        else {
            BASS_ChannelGetData(hdlStream, &fft, BASS_DATA_FFT2048);
            // "normal" FFT
            if (specMode == 0) {
                bitSpectrum->Canvas->Brush->Color = clBlack;
                bitSpectrum->Canvas->FillRect(Rect(0, 0, bitSpectrum->Width, bitSpectrum->Height));

                int nV;
                int nZ = Int((double)bitSpectrum->Width / 2);
                nZ = (nZ > 1024) ? 1024 : nZ;

                // bitSpectrum->Canvas->Pen->Color = TColor(RGB(50, 154, 255));
                for (int x = 0; x < nZ; x++) {
                    nY = Sqrt(fft[x]) * 3.3 * bitSpectrum->Height - 3; // scale it (sqrt to make low values more visible)
                    nY = (nY > bitSpectrum->Height) ? bitSpectrum->Height : nY;

                    bPeak[x] = (nY >= bPeak[x]) ? nY : bPeak[x] - 2;
                    bPeak[x] = (bPeak[x] < 0) ? 0 : bPeak[x];
                    bitSpectrum->Canvas->Pixels[x * 2 - 1][bitSpectrum->Height - bPeak[x]] = TColor(RGB(255, 240, 245));
                    bitSpectrum->Canvas->Pixels[x * 2][bitSpectrum->Height - bPeak[x]] = TColor(RGB(255, 240, 245));

                    /*
					aPeak[x] = (nY >= aPeak[x]) ? nY : aPeak[x] - 2;
					aPeak[x] = (aPeak[x] < 0) ? 0 : aPeak[x];
					bitSpectrum->Canvas->Pixels[x * 2][bitSpectrum->Height - aPeak[x]] = TColor(RGB(255, 240, 245));
					*/

                    if (x && (nV = (nY + nV) / 2)) // interpolate from previous to make the display smoother
                        while (--nV >= 0) {
                            /*
							bPeak[x] = (nV >= bPeak[x]) ? nV : bPeak[x] - 2;
							bPeak[x] = (bPeak[x] < 0) ? 0 : bPeak[x];
							bitSpectrum->Canvas->Pixels[x * 2 - 1][bitSpectrum->Height - bPeak[x]] = TColor(RGB(255, 240, 245));
							*/

                            nPcl = Int((double)nV / bitSpectrum->Height * 127);
                            bitSpectrum->Canvas->Pixels[x * 2 - 1][bitSpectrum->Height - nV] = TColor(RGB(nPal[nPcl][0], nPal[nPcl][1], nPal[nPcl][2]));
                            //bitSpectrum->Canvas->Pixels[x * 2 - 1][bitSpectrum->Height - bPeak[x]] = TColor(RGB(nPal[nPcl][0], nPal[nPcl][1], nPal[nPcl][2]));
                            //specBuf[v * SPECWIDTH + x * 2 - 1] = Int(v * SPECHEIGHT / 255) + 1;
                        }
                    nV = nY;
                    while (--nY >= 0) {
                        nPcl = Int((double)nY / bitSpectrum->Height * 127);
                        bitSpectrum->Canvas->Pixels[x * 2][bitSpectrum->Height - nY] = TColor(RGB(nPal[nPcl][0], nPal[nPcl][1], nPal[nPcl][2]));
                        //bitSpectrum->Canvas->Pixels[x * 2][bitSpectrum->Height - bPeak[x]] = TColor(RGB(nPal[nPcl][0], nPal[nPcl][1], nPal[nPcl][2]));
                        //specBuf[y * SPECWIDTH + x * 2] = Int(y * SPECHEIGHT / 255) + 1; // draw level
                    }
                }
            }
            // logarithmic, combine bins, fast
            else if (specMode == 1) {
                bitSpectrum->Canvas->Brush->Color = clBlack;
                bitSpectrum->Canvas->FillRect(Rect(0, 0, bitSpectrum->Width, bitSpectrum->Height));

                b0 = 0;
                for (int x = 0; x < BANDS; x++) {
                    peak = 0;
                    b1 = pow(2, x * 10.0 / (BANDS - 1));
                    if (b1 <= b0)
                        b1 = b0 + 1; // make sure it uses at least 1 FFT bin
                    if (b1 > 1023)
                        b1 = 1023;
                    for (; b0 < b1; b0++)
                        if (peak < fft[1 + b0])
                            peak = fft[1 + b0];
                    y = sqrt(peak) * 3.3 * bitSpectrum->Height - 3; // scale it (sqrt to make low values more visible)
                    if (y > bitSpectrum->Height)
                        y = bitSpectrum->Height; // cap it

                    aPeak[x] = (y >= aPeak[x]) ? y : aPeak[x] - 3;
                    aPeak[x] = (aPeak[x] < 0) ? 0 : aPeak[x];

                    bitSpectrum->Canvas->Pen->Color = TColor(RGB(255, 240, 245));
                    bitSpectrum->Canvas->MoveTo(x * (nW + 1), bitSpectrum->Height - aPeak[x]);
                    bitSpectrum->Canvas->LineTo(x * (nW + 1) + nW, bitSpectrum->Height - aPeak[x]);

                    nPcl = Int((double)y / bitSpectrum->Height * 127);
                    mRct = Rect(x * (nW + 1), bitSpectrum->Height - y, x * (nW + 1) + nW, bitSpectrum->Height);
                    GradientFillCanvas(bitSpectrum->Canvas, TColor(RGB(nPal[nPcl][0], nPal[nPcl][1], nPal[nPcl][2])), TColor(RGB(50, 154, 255)), mRct, gdVertical);

                    //while (--y >= 0) {
                    //	memset(specBuf + y * SPECWIDTH + x * (nW + 1), Int(y * SPECHEIGHT / 255) + 1, nW - 1); // draw bar
                    //}
                }
            }
            // logarithmic, combine bins, slowly falling
            else if (specMode == 2) {
                // bar spectrum, the original one with peak bar
                bitSpectrum->Canvas->Brush->Color = clBlack;
                bitSpectrum->Canvas->FillRect(Rect(0, 0, bitSpectrum->Width, bitSpectrum->Height));

                for (int i = 0; i < 128; i++) {
                    // nY = Int(abs(fft[i]) * 500);
                    nY = Sqrt(fft[i]) * 3.3 * bitSpectrum->Height - 3;

                    nY = (nY > bitSpectrum->Height) ? bitSpectrum->Height : nY;
                    fftPeak[i] = (nY >= fftPeak[i]) ? nY : fftPeak[i] - 1;
                    fftFall[i] = (nY >= fftFall[i]) ? nY : fftFall[i] - 3;
                    fftPeak[i] = (fftPeak[i] < 0) ? 0 : fftPeak[i];
                    fftFall[i] = (fftFall[i] < 0) ? 0 : fftFall[i];
                    /*
					bitSpectrum->Canvas->Pen->Color = TColor(RGB(255, 240, 245));
					bitSpectrum->Canvas->MoveTo(i * (nW + 1), bitSpectrum->Height - fftPeak[i]);
					bitSpectrum->Canvas->LineTo(i * (nW + 1) + nW, bitSpectrum->Height - fftPeak[i]);
                    */
                    for (int j = 1; j <= fftFall[i]; j++) {
                        nPcl = Int((double)j / bitSpectrum->Height * 127);
                        mRct = Rect(i * (nW + 1), bitSpectrum->Height - j, i * (nW + 1) + nW, bitSpectrum->Height);
                        GradientFillCanvas(bitSpectrum->Canvas, TColor(RGB(nPal[nPcl][0], nPal[nPcl][1], nPal[nPcl][2])), TColor(RGB(50, 154, 255)), mRct, gdVertical);
                    }
                    /*
					bitSpectrum->Canvas->Pen->Color = TColor(RGB(50, 154, 255));
					bitSpectrum->Canvas->Brush->Color = bitSpectrum->Canvas->Pen->Color;
					bitSpectrum->Canvas->Rectangle(i * (nW + 1), bitSpectrum->Height - fftFall[i], i * (nW + 1) + nW, bitSpectrum->Height);
					*/
                }
            }
            // "3d"
            else {
                for (int x = 0; x < bitSpectrum->Height; x++) {
                    nY = Sqrt(fft[x + 1]) * 3 * bitSpectrum->Height; // scale it (sqrt to make low values more visible)
                    nY = (nY > bitSpectrum->Height) ? bitSpectrum->Height : nY;

                    nPcl = 127 + Int((double)nY / bitSpectrum->Height * 127);
                    bitSpectrum->Canvas->Pixels[specPos][bitSpectrum->Height - x] = TColor(RGB(nPal[nPcl][0], nPal[nPcl][1], nPal[nPcl][2]));
                    //specBuf[x * SPECWIDTH + specPos] = 127 + y; // plot it
                }
                // move marker onto next position
                specPos = (specPos + 1) % bitSpectrum->Width;
                bitSpectrum->Canvas->Pen->Color = clWhite;
                bitSpectrum->Canvas->MoveTo(specPos, 0);
                bitSpectrum->Canvas->LineTo(specPos, bitSpectrum->Height);
            }
        }

        PaintBoxSpectrum->Repaint();
        // BitBlt(PaintBoxSpectrum->Canvas->Handle, 0, 0, PaintBoxSpectrum->Width, PaintBoxSpectrum->Height, bitSpectrum->Canvas->Handle, 0, 0, SRCCOPY);

#else // for windows gdi

        // Memo->Lines->Add(FormatDateTime(L"yy-mm-dd hh:nn:ss.zzz", Now()));

        // mono solid waveform
        if (specMode == 5) {
            buf = (float*)alloca(SPECWIDTH * sizeof(float)); // allocate buffer for data
            BASS_ChannelGetData(hdlStream, buf, SPECWIDTH * sizeof(float) | BASS_DATA_FLOAT); // get the sample data
            memset(specBuf, 0, SPECWIDTH * SPECHEIGHT);
            for (int x = 0; x < SPECWIDTH; x++) {
                v = (1 - buf[x]) * SPECHEIGHT / 2; // invert and scale to fit display
                if (v < 0)
                    v = 0;
                else if (v >= SPECHEIGHT)
                    v = SPECHEIGHT - 1;
                if (!x)
                    y = v;
                do { // draw line from previous sample...
                    if (y < v)
                        y++;
                    else if (y > v)
                        y--;
                    specBuf[y * SPECWIDTH + x] = 1;
                } while (y != v);
            }
        }
        // waveform
        else if (specMode == 4) {
            BASS_ChannelGetInfo(hdlStream, &ci); // get number of channels
            buf = (float*)alloca(ci.chans * SPECWIDTH * sizeof(float)); // allocate buffer for data
            BASS_ChannelGetData(hdlStream, buf, ci.chans * SPECWIDTH * sizeof(float) | BASS_DATA_FLOAT); // get the sample data
            memset(specBuf, 0, SPECWIDTH * SPECHEIGHT);
            for (int c = 0; c < ci.chans; c++) {
                for (int x = 0; x < SPECWIDTH; x++) {
                    v = (1 - buf[x * ci.chans + c]) * SPECHEIGHT / 2; // invert and scale to fit display
                    if (ci.chans > 1) {
                        v = (c & 1) ? v - 3 : v + 3;
                    }
                    if (v < 0)
                        v = 0;
                    else if (v >= SPECHEIGHT)
                        v = SPECHEIGHT - 1;
                    if (!x)
                        y = v;
                    do { // draw line from previous sample...
                        if (y < v)
                            y++;
                        else if (y > v)
                            y--;
                        if (ci.chans > 1) {
                            specBuf[y * SPECWIDTH + x] = (c & 1) ? 127 : 1;
                        }
                        else {
                            specBuf[y * SPECWIDTH + x] = 1;
                        }
                        //specBuf[y * SPECWIDTH + x] = c & 1 ? 127 : 1; // left=green, right=blue (could add more colours to palette for more chans)
                    } while (y != v);
                }
            }
        }
        else {
            BASS_ChannelGetData(hdlStream, &fft, BASS_DATA_FFT2048); // get the FFT data
            // "normal" FFT
            if (specMode == 0) {
                memset(specBuf, 0, SPECWIDTH * SPECHEIGHT);
                for (int x = 0; x < SPECWIDTH / 2; x++) {
    #if 1
                    y = sqrt(fft[x + 1]) * 3.3 * SPECHEIGHT - 3; // scale it (sqrt to make low values more visible)
    #else
                    y = fft[x + 1] * 10 * SPECHEIGHT; // scale it (linearly)
    #endif
                    if (y > SPECHEIGHT)
                        y = SPECHEIGHT; // cap it

                    dP[x] = (y >= dP[x]) ? y : dP[x] - 3;
                    dP[x] = (dP[x] <= 0) ? 0 : dP[x];
                    dP[x] = (dP[x] >= SPECHEIGHT) ? SPECHEIGHT - 1 : dP[x];
                    specBuf[dP[x] * SPECWIDTH + x * 2] = 255;

                    if (x && (v = (y + v) / 2)) // interpolate from previous to make the display smoother
                        while (--v >= 0)
                            specBuf[v * SPECWIDTH + x * 2 - 1] = Int(v * SPECHEIGHT / 255) + 1;
                    specBuf[dP[x] * SPECWIDTH + x * 2 - 1] = 255;
                    v = y;
                    while (--y >= 0)
                        specBuf[y * SPECWIDTH + x * 2] = Int(y * SPECHEIGHT / 255) + 1; // draw level
                }
            }
            // logarithmic, combine bins, fast
            else if (specMode == 1) {
                b0 = 0;
                int sS;
                memset(specBuf, 0, SPECWIDTH * SPECHEIGHT);
                for (int x = 0; x < BANDS; x++) {
                    peak = 0;
                    b1 = pow(2, x * 10.0 / (BANDS - 1));
                    if (b1 <= b0)
                        b1 = b0 + 1; // make sure it uses at least 1 FFT bin
                    if (b1 > 1023)
                        b1 = 1023;
                    for (; b0 < b1; b0++)
                        if (peak < fft[1 + b0])
                            peak = fft[1 + b0];
                    y = sqrt(peak) * 3.3 * SPECHEIGHT - 3; // scale it (sqrt to make low values more visible)
                    if (y > SPECHEIGHT)
                        y = SPECHEIGHT; // cap it

                    dP[x] = (y >= dP[x]) ? y : dP[x] - 3;
                    dP[x] = (dP[x] <= 0) ? 0 : dP[x];
                    dP[x] = (dP[x] >= SPECHEIGHT) ? SPECHEIGHT - 1 : dP[x];
                    memset(specBuf + dP[x] * SPECWIDTH + x * (nW + 1), 127, nW - 1); // draw line

                    while (--y >= 0)
                        memset(specBuf + y * SPECWIDTH + x * (nW + 1), Int(y * SPECHEIGHT / 255) + 1, nW - 1); // draw bar
                }
            }
            // logarithmic, combine bins, slowly falling
            else if (specMode == 2) {
                b0 = 0;
                memset(specBuf, 0, SPECWIDTH * SPECHEIGHT);
                for (int x = 0; x < BANDS; x++) {
                    peak = 0;
                    b1 = pow(2, x * 10.0 / (BANDS - 1));
                    if (b1 <= b0)
                        b1 = b0 + 1; // make sure it uses at least 1 FFT bin
                    if (b1 > 1023)
                        b1 = 1023;
                    for (; b0 < b1; b0++)
                        if (peak < fft[x + b0])
                            peak = fft[x + b0];
                    y = sqrt(peak) * 3.3 * SPECHEIGHT - 3; // scale it (sqrt to make low values more visible)
                    if (y > SPECHEIGHT)
                        y = SPECHEIGHT; // cap it

                    dP[x] = (y >= dP[x]) ? y : dP[x] - 3;
                    dP[x] = (dP[x] <= 0) ? 0 : dP[x];
                    dP[x] = (dP[x] >= SPECHEIGHT) ? SPECHEIGHT - 1 : dP[x];
                    y = dP[x];

                    while (--y >= 0)
                        memset(specBuf + y * SPECWIDTH + x * (nW + 1), Int(y * SPECHEIGHT / 255) + 1, nW - 1); // draw bar
                }
            }
            // "3d"
            else {
                for (int x = 0; x < SPECHEIGHT; x++) {
                    y = sqrt(fft[x + 1]) * 3 * SPECHEIGHT; // scale it (sqrt to make low values more visible)
                    if (y > SPECHEIGHT) {
                        y = SPECHEIGHT; // cap it
                    }
                    specBuf[x * SPECWIDTH + specPos] = 127 + y; // plot it
                }
                // move marker onto next position
                specPos = (specPos + 1) % SPECWIDTH;
                for (int x = 0; x < SPECHEIGHT; x++) {
                    specBuf[x * SPECWIDTH + specPos] = 255;
                }
            }
        }
        dc = GetDC(win);
        BitBlt(PaintBoxSpectrum->Canvas->Handle, 0, 0, SPECWIDTH, SPECHEIGHT, specDc, 0, 0, SRCCOPY);
        ReleaseDC(win, dc);
#endif
    }
    // stop playing
    else {
#if flgBuilder
        if (specMode == 3) {
            specPos = 0;
        }
        bitSpectrum->Canvas->Brush->Color = clBlack;
        bitSpectrum->Canvas->FillRect(Rect(0, 0, bitSpectrum->Width, bitSpectrum->Height));
        PaintBoxSpectrum->Repaint();
#else
        if (specMode == 3) {
            specPos = 0;
        }
        memset(specBuf, 0, SPECWIDTH * SPECHEIGHT);
        dc = GetDC(win);
        BitBlt(PaintBoxSpectrum->Canvas->Handle, 0, 0, SPECWIDTH, SPECHEIGHT, specDc, 0, 0, SRCCOPY);
        ReleaseDC(win, dc);
#endif
        flgPlay = false;
        TmrPos->Enabled = false;
        ShapePos->Left = -1;
        BtnPlay->Caption = L"&Play";
        LabelLeft->Width = 0;
        LabelRight->Width = 0;
        MainForm->Caption = Format(L"%s    ❤    %s", ARRAYOFCONST((caption, mTime)));
    }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TmrColorTimer(TObject* Sender)
{
    //Memo->Lines->Add(nCl);
    //Memo->Lines->Add(FormatDateTime(L"yy-mm-dd hh:nn:ss.zzz", Now()));
    if (nCl > 127 && (nCl - 128) % 32 == 0) {
        Memo->Lines->Add(L"----------x----------x----------x----------");
    }
#if flgBuilder
    Memo->Lines->Add(Format(L"%s --> RGB(%d, %d, %d)", ARRAYOFCONST((Format(L"%3d", ARRAYOFCONST((nCl))), nPal[nCl][0], nPal[nCl][1], nPal[nCl][2]))));
#else
    //extern RGBQUAD* pal; // global variable, but failed
    //Memo->Lines->Add(Format(L"%s --> RGB(%d, %d, %d)", ARRAYOFCONST((Format(L"%3d", ARRAYOFCONST((nCl))), pal[nCl].rgbGreen, pal[nCl].rgbGreen, pal[nCl].rgbBlue))));
    //Memo->Lines->Add(Format(L"%s --> RGB(%d, %d, %d)", ARRAYOFCONST((Format(L"%3d", ARRAYOFCONST((nCl))), (int)pal[nCl].rgbRed, (int)pal[nCl].rgbGreen, (int)pal[nCl].rgbBlue))));
#endif

    //    bitSpectrum->Width = PaintBoxSpectrum->Width;
    //    bitSpectrum->Height = PaintBoxSpectrum->Height;

    for (int x = 0; x <= BANDS; x++) {
        nPcl = nCl + x;

#if flgBuilder
        bitSpectrum->Canvas->Brush->Color = TColor(RGB(nPal[nPcl][0], nPal[nPcl][1], nPal[nPcl][2]));
        bitSpectrum->Canvas->Rectangle(x * (nW + 0), 0, x * (nW + 0) + nW, PaintBoxSpectrum->Height);
#else
        y = SPECHEIGHT;
        while (--y >= 0)
            memset(specBuf + y * SPECWIDTH + x * (nW + 0), nPcl, nW - 1); // draw bar
#endif
    }

#if flgBuilder
    PaintBoxSpectrum->Repaint();
#else
    dc = GetDC(win);
    BitBlt(PaintBoxSpectrum->Canvas->Handle, 0, 0, SPECWIDTH, SPECHEIGHT, specDc, 0, 0, SRCCOPY);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = 10;
    bmi.bmiHeader.biHeight = 10;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    RGBQUAD* p = new RGBQUAD[10 * 10];
    GetDIBits(dc, specBmp, 2, 2, p, &bmi, DIB_RGB_COLORS);
    getPal(*p);
    delete[] p;

    ReleaseDC(win, dc);
#endif

    nCl++;
    if (nCl > 255) {
        nCl = 0;
        flgPlay = false;
        TmrColor->Enabled = false;
        BtnPlay->Caption = L"&Play";
        Memo->Lines->Add(L"stop");
    }
}
//---------------------------------------------------------------------------
// callback function no need declare in main.h
// declare and call it in main.cpp directly
// callback function, must put it before where calls the function
void CALLBACK syncProc(HSYNC handle, DWORD channel, DWORD data, void* user)
{
    if (!MainForm->ShapeStart->Visible) {
        posStart = 0;
    }

    BASS_ChannelSetPosition(hdlStream, posStart, BASS_POS_BYTE);
    BASS_ChannelPlay(hdlStream, FALSE);
    // BASS_ChannelPlay(hdlStream, TRUE);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::PaintBoxLevelMouseDown(TObject* Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if (Shift.Contains(ssCtrl) && Button == mbLeft) {
        ShapeStart->Left = X;
        ShapeStart->Visible = true;
        posStart = Int((double)ShapeStart->Left / PaintBoxLevel->Width * mp3Len);
    }
    else if (Shift.Contains(ssCtrl) && Button == mbRight) {
        ShapeEnd->Left = X;
        ShapeEnd->Visible = true;
        posEnd = Int((double)ShapeEnd->Left / PaintBoxLevel->Width * mp3Len);
        BASS_ChannelRemoveSync(hdlStream, hdlSync);
        hdlSync = BASS_ChannelSetSync(hdlStream, BASS_SYNC_POS, posEnd, &syncProc, 0);
    }
    //else if (Button == mbMiddle) {
    else if (!Shift.Contains(ssCtrl) && (Button == mbLeft || Button == mbRight)) {
        ShapePos->Left = X;
        BASS_ChannelSetPosition(hdlStream, Int((double)X / PaintBoxLevel->Width * mp3Len), BASS_POS_BYTE);
        flgPlay = false;
        BtnPlay->Click();
    }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::PaintBoxLevelPaint(TObject* Sender)
{
    PaintBoxLevel->Canvas->StretchDraw(Bounds(0, 0, PaintBoxLevel->Width, PaintBoxLevel->Height), bitLevel);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::PaintBoxSpectrumPaint(TObject* Sender)
{
    PaintBoxSpectrum->Canvas->StretchDraw(Bounds(0, 0, PaintBoxSpectrum->Width, PaintBoxSpectrum->Height), bitSpectrum);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::PaintBoxSpectrumClick(TObject* Sender)
{
    if (!flgPlay) {
        return;
    }
    specMode = (specMode + 1) % 6; // change spectrum mode;
#if flgBuilder
    if (specMode == 3) {
        specPos = 0;
        bitSpectrum->Canvas->Brush->Color = clBlack;
        bitSpectrum->Canvas->FillRect(Rect(0, 0, bitSpectrum->Width, bitSpectrum->Height));
    }
#else
    if (specMode == 3) {
        specPos = 0;
        memset(specBuf, 0, SPECWIDTH * SPECHEIGHT);
    }
#endif
}
//---------------------------------------------------------------------------

