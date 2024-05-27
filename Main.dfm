object MainForm: TMainForm
  Left = 0
  Top = 0
  Caption = 'Wave'
  ClientHeight = 282
  ClientWidth = 708
  Color = clBtnFace
  Constraints.MaxHeight = 320
  Constraints.MinHeight = 320
  Constraints.MinWidth = 720
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poDesktopCenter
  OnCreate = FormCreate
  OnResize = FormResize
  OnShow = FormShow
  TextHeight = 15
  object ShapeLeft: TShape
    Left = 0
    Top = 100
    Width = 708
    Height = 22
    Align = alTop
    Brush.Color = clBtnFace
    Constraints.MaxHeight = 22
    Constraints.MinHeight = 22
    Pen.Color = clSkyBlue
    Pen.Style = psClear
    ExplicitTop = 95
  end
  object ShapeRight: TShape
    Left = 0
    Top = 122
    Width = 708
    Height = 22
    Align = alTop
    Brush.Color = clBtnFace
    Constraints.MaxHeight = 22
    Constraints.MinHeight = 22
    Pen.Color = clSkyBlue
    Pen.Style = psClear
    ExplicitLeft = 1
    ExplicitTop = 127
  end
  object PaintBoxLevel: TPaintBox
    Left = 0
    Top = 0
    Width = 708
    Height = 100
    Hint = 
      'Left_Click: Jump to current position and play; Ctrl+Left_Click: ' +
      'Set the start point; Ctrl+Right_Click: Set the end point.'
    Align = alTop
    Constraints.MaxHeight = 100
    Constraints.MinHeight = 100
    ParentShowHint = False
    ShowHint = True
    OnMouseDown = PaintBoxLevelMouseDown
    OnPaint = PaintBoxLevelPaint
    ExplicitTop = -4
  end
  object ShapePos: TShape
    Left = 8
    Top = 14
    Width = 65
    Height = 65
  end
  object ShapeStart: TShape
    Left = 85
    Top = 14
    Width = 65
    Height = 65
  end
  object ShapeEnd: TShape
    Left = 165
    Top = 14
    Width = 65
    Height = 65
  end
  object PaintBoxSpectrum: TPaintBox
    Left = 0
    Top = 144
    Width = 708
    Height = 100
    Hint = 'Left_Click: Change the Spectrum Mode.'
    Align = alTop
    Constraints.MaxHeight = 100
    Constraints.MinHeight = 100
    ParentShowHint = False
    ShowHint = True
    OnClick = PaintBoxSpectrumClick
    OnPaint = PaintBoxSpectrumPaint
    ExplicitTop = 148
  end
  object LabelRight: TLabel
    Left = 0
    Top = 123
    Width = 128
    Height = 20
    AutoSize = False
    Caption = 'Right'
    Color = clSkyBlue
    Constraints.MaxHeight = 20
    Constraints.MinHeight = 20
    ParentColor = False
    Transparent = False
  end
  object LabelLeft: TLabel
    Left = 0
    Top = 102
    Width = 108
    Height = 20
    AutoSize = False
    Caption = 'Left'
    Color = clSkyBlue
    Constraints.MaxHeight = 20
    Constraints.MinHeight = 20
    ParentColor = False
    Transparent = False
  end
  object LabelHint: TLabel
    Left = 8
    Top = 259
    Width = 3
    Height = 15
  end
  object BtnPlay: TButton
    Left = 535
    Top = 254
    Width = 75
    Height = 25
    Hint = 'Play or Pause Playing the audio.  P'
    Caption = '&Play'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 0
    OnClick = BtnPlayClick
  end
  object BtnOpen: TButton
    Left = 454
    Top = 254
    Width = 75
    Height = 25
    Hint = 'Open an audio file.  O'
    Caption = '&Open'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 2
    OnClick = BtnOpenClick
  end
  object BtnRemove: TButton
    Left = 616
    Top = 254
    Width = 75
    Height = 25
    Hint = 'Remove the start and end points.  R'
    Caption = '&Remove'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 1
    OnClick = BtnRemoveClick
  end
  object Memo: TMemo
    Left = 375
    Top = 0
    Width = 333
    Height = 100
    ScrollBars = ssVertical
    TabOrder = 3
    Visible = False
  end
  object ChkLoop: TCheckBox
    Left = 320
    Top = 257
    Width = 113
    Height = 17
    Hint = 'Play the music over and over.  L'
    Caption = '&Looping music'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 4
    OnClick = ChkLoopClick
  end
  object TmrPos: TTimer
    Enabled = False
    Interval = 50
    OnTimer = TmrPosTimer
    Left = 245
    Top = 175
  end
  object OpenDialog: TOpenDialog
    Left = 145
    Top = 175
  end
  object TmrColor: TTimer
    Enabled = False
    Interval = 100
    OnTimer = TmrColorTimer
    Left = 335
    Top = 175
  end
end
