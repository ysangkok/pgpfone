/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CLevelMeter.h,v 1.4 1999/03/10 02:34:35 heller Exp $
____________________________________________________________________________*/
#ifndef CLEVELMETER_H
#define CLEVELMETER_H

#define LEVELMETER_NUM_LEDS			16
#define LEVELMETER_LED_WIDTH		 3
#define LEVELMETER_DELIMITER_WIDTH	 1
#define LEVELMETER_HEIGHT			18

class CLevelMeter : public CButton
{
public:
				CLevelMeter();
	virtual 	~CLevelMeter();

	void		SetLevel(long level);
	void		SetStatus(short on);
	CRect       LevelMeterRectangle(void);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLevelMeter)
	public:
	virtual BOOL Create(LPCTSTR lpszCaption, 
						DWORD dwStyle, 
						const POINT& topLeft, 
						CWnd* pParentWnd, 
						UINT nID,
						short numLEDs,
						short LEDWidth,
						short delimiterWidth,
						short LEDHeight);

	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CLevelMeter)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	/*These are the bitmaps used to build the meter.  We don't dynamically 
	 *draw them, because building them beforehand and copying them over each 
	 *other is MUCH faster, and we need to do this a lot.
	 *
	 *mBaseMeter is the completely empty meter everything else is drawn on (a
	 *           black rectangle with silver lines on the edges to make it
	 *           look "3D")
	 *mGreyMeter is the greyed-out LED meter
	 *mDarkMeter is the on-but-no-sound meter
	 *mLitMeter  is the 100% full-on meter
	 */
	CBitmap		mBaseMeter, mGreyMeter, mDarkMeter, mLitMeter;
	BOOLEAN		mInit, mOn;
	long		mLevel;
	short       mNumLEDs, mLEDWidth, mDelimiterWidth, mLEDHeight;
	CPoint      mLeftTop;

	//Makes an LED meter based upon the Create params and the colors passed:
	void 		MakeLEDMeter(CBitmap *bitmap, 
								   CPaintDC *dc, 
								   CRect *rect,
								   COLORREF LEDColor,
								   COLORREF highLEDColor,
								   COLORREF delimiterColor);

};

#endif

