/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CXferWindow.h,v 1.9 1999/03/10 02:42:05 heller Exp $
____________________________________________________________________________*/

#ifndef CXFERWINDOW_H
#define CXFERWINDOW_H

#include "resource.h"
#include "afxole.h" //For the drag-n-drop stuff

#include "CXferThread.h"
#include "CWinFilePipe.h"

class CMessageQueue;
struct XSendFile;
struct XRcvFile;
struct XferInfo;

/////////////////////////////////////////////////////////////////////////////
// CXferWindow dialog

class CXferWindow : public CWnd
{
// Construction
public:
	CXferWindow();
	
	void				SetXferThread(CXferThread *xferThread);
	void 				SendFile(XferFileSpec* fs);
	void				SendFolder(XferFileSpec* fs);
	BOOL				FileIsAFolder(char* path);
	void				StartSend(XferInfo *xi);
	void				StartReceive(XferInfo *xi);
	void				EndSend();
	void				EndReceive();
	void				QueueSend(XSendFile *nxs);
	void				QueueReceive(XRcvFile *nxr);
	void				DequeueSend(XSendFile *nxs);
	void				DequeueReceive(XRcvFile *nxr);
	
	HRESULT 			ResolveLink(	LPCSTR lpszLinkFile, 
										LPSTR lpszPath);
// Dialog Data
	//{{AFX_DATA(CXferWindow)
	enum { IDD = IDD_FTP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXferWindow)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CXferWindow)
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT TimerID);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
	void GetFileIcon(	char* szFilePath,
						HICON* largeIcon,
						HICON* smallIcon);
						
	XferInfo* mSendXferInfo;
	XferInfo* mRecvXferInfo;
	
	CImageList mSendSmallImageList;
	CImageList mRecvSmallImageList;
	
	CRITICAL_SECTION mSendCriticalSection;
	CRITICAL_SECTION mRecvCriticalSection;
	
	CListCtrl		mSendList;
	CListCtrl 		mRecvList;
	CStatic 		mSendFile;
	CStatic			mRecvFile;
	CStatic 		mSendSize;
	CStatic 		mRecvSize;
	CProgressCtrl 	mSendProgress;
	CProgressCtrl 	mRecvProgress;
	CButton			mSendCancel;
	CButton			mRecvCancel;
	CButton 		mSendGroup;
	CButton 		mRecvGroup;
	
	HICON mSendLargeIcon, mSendSmallIcon;
	HICON mRecvLargeIcon, mRecvSmallIcon;
	
	CXferThread			*mXfer;
	CMessageQueue		*mXferQueue;
	
	XRcvFile			*mQReceives[MAXRCVFILES];
	XSendFile			*mQSends[MAXSNDFILES];
	
	long 				mNumSends, mNumReceives;
	
	
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#endif

