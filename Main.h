//---------------------------------------------------------------------------

#ifndef MainH
#define MainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ComCtrls.hpp>
//---------------------------------------------------------------------------
class TMainForm : public TForm
{
  __published: // IDE-managed Components
    TOpenDialog* OpenDialog;
    TPaintBox* PaintBoxSpectrum;
    TPaintBox* PaintBoxLevel;
    TButton* BtnOpen;
    TButton* BtnPlay;
    TButton* BtnRemove;
    TShape* ShapeRight;
    TShape* ShapeLeft;
    TShape* ShapePos;
    TShape* ShapeStart;
    TShape* ShapeEnd;
    TTimer* TmrPos;
    TTimer* TmrColor;
    TLabel* LabelLeft;
    TLabel* LabelRight;
    TLabel* LabelHint;
    TCheckBox* ChkLoop;
    TMemo* Memo;
    void __fastcall PaintBoxLevelMouseDown(TObject* Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall PaintBoxSpectrumPaint(TObject* Sender);
    void __fastcall PaintBoxSpectrumClick(TObject* Sender);
    void __fastcall PaintBoxLevelPaint(TObject* Sender);
    void __fastcall FormCreate(TObject* Sender);
    void __fastcall FormShow(TObject* Sender);
    void __fastcall FormResize(TObject* Sender);
    void __fastcall BtnOpenClick(TObject* Sender);
    void __fastcall BtnPlayClick(TObject* Sender);
    void __fastcall BtnRemoveClick(TObject* Sender);
    void __fastcall TmrPosTimer(TObject* Sender);
    void __fastcall TmrColorTimer(TObject* Sender);
    void __fastcall ChkLoopClick(TObject* Sender);
  private: // User declarations
  public: // User declarations
    __fastcall TMainForm(TComponent* Owner);
    __fastcall ~TMainForm(void);

    // own function ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    UnicodeString __fastcall secToMark(double nSec);
    void __fastcall getPal(RGBQUAD &p);
    void __fastcall initial(void);
    void __fastcall openFile(void);
    void __fastcall drawPeak(void);
};
//---------------------------------------------------------------------------
extern PACKAGE TMainForm* MainForm;
//---------------------------------------------------------------------------
#endif

