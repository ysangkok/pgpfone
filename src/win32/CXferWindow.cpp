/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CXferWindow.cpp,v 1.11 1999/03/10 02:42:04 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include <string.h>

#define INITGUID
#include <shlobj.h>
#undef INITGUID

#include <shellapi.h>
#include <Afxcmn.h>

#include "PGPFone.h"
#include "CPGPFone.h"
#include "PGPFWinUtils.h"
#include "CXferWindow.h"
#include "CMessageQueue.h"
#include "CWinFilePipe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define REFRESH_TIMER 1000

static char szNoSend[]="Drop files or folders to send.";
static char szNoRecv[]="Nothing to receive.";

extern CPGPFoneApp theApp;

typedef struct ItemInfo
{	
	union
	{
		XRcvFile* xrf;
		XSendFile* xsf;
	}xf;
	
	int iconIndex;
}ItemInfo;

#define  xsf xf.xsf 
#define  xrf xf.xrf 

/////////////////////////////////////////////////////////////////////////////
// CXferWindow dialog


CXferWindow::CXferWindow()
{
	//{{AFX_DATA_INIT(CXferWindow)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	
	mXferQueue 		= NULL;
	mXfer 			= NULL;
	mNumSends 		= 0;
	mNumReceives 	= 0;
	mSendXferInfo	= NULL;
	mRecvXferInfo	= NULL;
	
}


void CXferWindow::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CXferWindow)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
// CXferWindow message handlers

int CXferWindow::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	HFONT hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT ); 
	
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	InitializeCriticalSection(&mSendCriticalSection);
	InitializeCriticalSection(&mRecvCriticalSection);
	
	const RECT rSendGroup 		= {6, 	0, 	180, 101};
	const RECT rRecvGroup 		= {178, 0, 	353, 101};
	const RECT rSendList 		= {5, 	106,178, 260};
	const RECT rRecvList		= {180, 106,354, 260};
	const RECT rSendProgress 	= {30, 	54, 155, 69};
	const RECT rRecvProgress	= {201, 54, 326, 69};
	const RECT rSendCancel 		= {63, 	75, 122, 96};
	const RECT rRecvCancel		= {235, 75, 294, 96};
	const RECT rSendFile 		= {30, 	15, 170, 28};
	const RECT rRecvFile		= {201, 15, 341, 28};
	const RECT rSendSize 		= {30, 	35, 170, 48};
	const RECT rRecvSize		= {201, 35, 341, 48};
	
	mSendGroup.Create(	"Send",
						WS_CHILD|WS_VISIBLE|BS_GROUPBOX,
						rSendGroup,
						this, IDC_STATIC);
						
	mSendGroup.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);	
					
	mRecvGroup.Create(	"Receive",
						WS_CHILD|WS_VISIBLE|BS_GROUPBOX,
						rRecvGroup,
						this, IDC_STATIC);
						
	mRecvGroup.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);	
						
	mSendCancel.Create(	"Cancel",
						WS_CHILD|WS_VISIBLE,
						rSendCancel,
						this, ID_CANCEL_SEND);
						
	mSendCancel.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);	
					
	mRecvCancel.Create(	"Cancel",
						WS_CHILD|WS_VISIBLE,
						rRecvCancel,
						this, ID_CANCEL_RECV);
						
	mRecvCancel.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);
	
	const RECT cListCtrlRect = {rSendList.left,
						rSendList.top,
						rSendList.right - rSendList.left,
						rSendList.bottom - rSendList.top };
	
	mSendList.CreateEx( WS_EX_CLIENTEDGE,
						LVS_SMALLICON | LVS_SINGLESEL | 
						WS_BORDER | WS_TABSTOP | WS_VISIBLE |
						WS_VISIBLE | WS_CHILD, 
						cListCtrlRect,
						GetSafeHwnd(), 
						(HMENU)IDC_SEND_LIST);
						
	mSendList.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);
	
	mRecvList.CreateEx( WS_EX_CLIENTEDGE,
						WC_LISTVIEW,
						"",
						LVS_SMALLICON | LVS_SINGLESEL | 
						WS_BORDER | WS_TABSTOP | WS_VISIBLE |
						WS_VISIBLE | WS_CHILD, 
						rRecvList.left,
						rRecvList.top,
						rRecvList.right - rRecvList.left,
						rRecvList.bottom - rRecvList.top,
						GetSafeHwnd(), 
						(HMENU)IDC_RECV_LIST, 
						NULL);
						
	mRecvList.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);
						
	mSendSmallImageList.Create( 16, 16, 
								ILC_MASK|ILC_COLOR8, 
								5, 1024 );
								
	mRecvSmallImageList.Create( 16, 16, 
								ILC_MASK|ILC_COLOR8, 
								5, 1024 );
								
	mSendList.SetImageList(&mSendSmallImageList, LVSIL_SMALL);
	mRecvList.SetImageList(&mRecvSmallImageList, LVSIL_SMALL);
						
	mSendFile.Create(	"",
						WS_CHILD|WS_VISIBLE,
						rSendFile, this, IDC_SEND_FILE);
						
	mSendFile.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);
	
	mRecvFile.Create(	"",
						WS_CHILD|WS_VISIBLE,
						rRecvFile, this, IDC_RECV_FILE);
						
	mRecvFile.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);
						
	mSendSize.Create(	"",
						WS_CHILD|WS_VISIBLE,
						rSendSize, this, IDC_SEND_SIZE);
						
	mSendSize.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);
						
	mRecvSize.Create(	"",
						WS_CHILD|WS_VISIBLE,
						rRecvSize, this, IDC_RECV_SIZE);
						
	mRecvSize.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);	
				
	mSendProgress.Create( 	WS_CHILD|WS_VISIBLE,
							rSendProgress, this, IDC_SEND_PROGRESS);
						
	mSendProgress.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);
						
	mRecvProgress.Create(	WS_CHILD|WS_VISIBLE,
							rRecvProgress, this, IDC_RECV_PROGRESS);
						
	mRecvProgress.SendMessage(WM_SETFONT, (WPARAM)hFont, FALSE);					
	
	mSendFile.SetWindowText(szNoSend);
	mSendSize.SetWindowText("");
	mSendProgress.SetRange(0, 100);
	mSendProgress.SetPos(0);
	
	mRecvFile.SetWindowText(szNoRecv);
	mRecvSize.SetWindowText("");
	mRecvProgress.SetRange(0, 100);
	mRecvProgress.SetPos(0);
	
	SetTimer(REFRESH_TIMER, 1000, NULL ); // refresh display each second
	
	return 0;
}


BOOL CXferWindow::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch(LOWORD(wParam))
	{
		case ID_CANCEL_RECV:
		{
			if(mRecvXferInfo)
			{
				mXferQueue->Send(_mt_abortReceive);
			}
		}
		
		case ID_CANCEL_SEND:
		{
			if(mSendXferInfo)
			{
				mXferQueue->Send(_mt_abortSend);
			}
		}
	}
	
	return CWnd::OnCommand(wParam, lParam);
}

void CXferWindow::OnDropFiles(HDROP hDropInfo) 
{
	UINT numFiles, count;
	char szFilePath[MAX_PATH + 1];
	XferFileSpec fs;
	
	numFiles = DragQueryFile( 	hDropInfo, 
								-1,
								szFilePath,
								sizeof(szFilePath));
							
	count = 0;
								
	while(count < numFiles)
	{
		DragQueryFile( 	hDropInfo, 
						count,
						szFilePath,
						sizeof(szFilePath));
						
		if(!strcmp((szFilePath + strlen(szFilePath) - 4), ".lnk"))
		{
			char szLinkFile[MAX_PATH + 1];
			
			strcpy(szLinkFile, szFilePath);
		
			ResolveLink(szLinkFile, szFilePath);
		}
		
		strcpy(fs.path, szFilePath);
		
		fs.filename = strrchr(fs.path, '\\' );
		fs.filename++;
				
		if(FileIsAFolder(szFilePath))
		{
			fs.root = fs.filename;
			
			SendFolder(&fs);
		}
		else
		{
			fs.root = NULL;
			SendFile(&fs);
		}

		count++;
	}
	
	CWnd::OnDropFiles(hDropInfo);
}

void CXferWindow::OnDestroy() 
{
	int index;
	KillTimer( REFRESH_TIMER );
	
	EnterCriticalSection(&mSendCriticalSection);
	EnterCriticalSection(&mRecvCriticalSection);
	
	int count = mSendList.GetItemCount();
	
	ItemInfo* info;
	
	for(index = 0; index < count; index++)
	{
		info = (ItemInfo*) mSendList.GetItemData(index);

		if( info )
		{
			delete info;
		}
	}
	
	count = mRecvList.GetItemCount();
	
	for(index = 0; index < count; index++)
	{
		info = (ItemInfo*) mRecvList.GetItemData(index);

		if( info )
		{
			delete info;
		}
	}

	
	mSendList.DeleteAllItems();
	mRecvList.DeleteAllItems();
	
	mSendSmallImageList.DeleteImageList();					
	mRecvSmallImageList.DeleteImageList();
	
	DeleteCriticalSection(&mSendCriticalSection);
	DeleteCriticalSection(&mRecvCriticalSection);
								
	CWnd::OnDestroy();
}

void CXferWindow::OnClose() 
{
	ShowWindow(SW_HIDE);
}


void CXferWindow::OnTimer(UINT TimerID)
{
	switch(TimerID)
	{
		case REFRESH_TIMER:
		{
			char string[256];
			float percentDone;
			
			EnterCriticalSection(&mSendCriticalSection);
		
			if(mSendXferInfo)
			{
				percentDone = 	((float)mSendXferInfo->bytesDone)/
								((float)mSendXferInfo->bytesTotal)*100;
				
				if(mSendXferInfo->bytesTotal < 1024)
				{
					wsprintf(string,"%lu of %lu Bytes (%d%%)", 
									mSendXferInfo->bytesDone, 
									mSendXferInfo->bytesTotal,
									(int)percentDone);
				}
				else
				{
					wsprintf(string,"%lu of %lu K (%d%%)", 
									mSendXferInfo->bytesDone/1024, 
									mSendXferInfo->bytesTotal/1024,
									(int)percentDone);			
				}
				
				mSendSize.SetWindowText(string);
		
				mSendProgress.SetPos( (int)percentDone );
			}
			
			if(!mNumSends)
			{	
				int count = mSendSmallImageList.GetImageCount();
				
				for(int i = 0; i < count; i++)
				{
					mSendSmallImageList.Remove(i);
				}
			}
			
			LeaveCriticalSection(&mSendCriticalSection);
			
			EnterCriticalSection(&mRecvCriticalSection);
			
			if(mRecvXferInfo)
			{
				percentDone = 	((float)mRecvXferInfo->bytesDone)/
								((float)mRecvXferInfo->bytesTotal)*100;
				
				if(mRecvXferInfo->bytesTotal < 1024)
				{
					wsprintf(string,"%lu of %lu Bytes (%d%%)", 
									mRecvXferInfo->bytesDone, 
									mRecvXferInfo->bytesTotal,
									(int)percentDone);
				}
				else
				{
					wsprintf(string,"%lu of %lu K (%d%%)", 
									mRecvXferInfo->bytesDone/1024, 
									mRecvXferInfo->bytesTotal/1024,
									(int)percentDone);						
				}
				
				mRecvSize.SetWindowText(string);
		
				mRecvProgress.SetPos((int)percentDone );
			}
			
			if(!mNumReceives)
			{
				int count = mRecvSmallImageList.GetImageCount();
				
				for(int i = 0; i < count; i++)
				{
					mRecvSmallImageList.Remove(i);
				}
			}
			
			LeaveCriticalSection(&mRecvCriticalSection);
			
			break;
		}
	}

	CWnd::OnTimer(TimerID);
}
              
void CXferWindow::GetFileIcon(	char* szFilePath,
								HICON* largeIcon,
								HICON* smallIcon)
{
	SHFILEINFO shellinfo;
	
	if(!SHGetFileInfo( 	szFilePath, 
						0,
						&shellinfo,
						sizeof(SHFILEINFO),
						SHGFI_ICON | SHGFI_SMALLICON ))
	{
		AfxMessageBox("Failed to Get Info.");
	}
	else
	{
		if(smallIcon)
		{
			*smallIcon = shellinfo.hIcon;
		}
		
		if(largeIcon)
		{
			if(SHGetFileInfo( 	szFilePath, 
						0,
						&shellinfo,
						sizeof(SHFILEINFO),
						SHGFI_ICON | SHGFI_LARGEICON ))
			{
				*largeIcon = shellinfo.hIcon;
			}
			
		
		}
		
		
 	}
}

BOOL CXferWindow::FileIsAFolder(char* path)
{
	char current[MAX_PATH];
	
	GetCurrentDirectory(sizeof(current), current);
	
	if(SetCurrentDirectory(path))
	{
		SetCurrentDirectory(current);
		
		return TRUE;
	}
	
	return FALSE;

}

/////////////////////////////////////////////////////////////////////////////

void CXferWindow::SetXferThread(CXferThread *xferThread)
{
	mXfer = xferThread;
	
	if(xferThread)
	{
		mXferQueue = xferThread->GetQueue();
	}
	else
	{
		mXferQueue = NULL;
	}
}

void CXferWindow::SendFolder(XferFileSpec* fs)
{
	HANDLE handle;
	WIN32_FIND_DATA wfd;
	char find[MAX_PATH];
	
	strcpy(find, fs->path);
	strcat(find, "\\*.*");
	
	handle = FindFirstFile(find, &wfd);
	
	if( INVALID_HANDLE_VALUE != handle )
	{
		do
		{
			if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if( strcmp(wfd.cFileName, ".") && 
					strcmp(wfd.cFileName, "..") )
				{
					XferFileSpec dirfs;
					
					strcpy(dirfs.path, fs->path);
					strcat(dirfs.path, "\\");
					strcat(dirfs.path, wfd.cFileName);
					
					//dirfs.filename = strrchr(dirfs.path, '\\' );
					//dirfs.filename++;
			
					dirfs.root = dirfs.path + (fs->root - fs->path);
					
					SendFolder(&dirfs);
				}
			}
			else
			{
				strcat(fs->path, "\\");
				strcat(fs->path, wfd.cFileName);
		
				fs->filename = strrchr(fs->path, '\\' );
				fs->filename++;
		
				SendFile(fs);
			}
		
		}while( FindNextFile(handle,&wfd) );
	}
	else
	{
		char szMessage[MAX_PATH + 25];
		
		wsprintf(szMessage, "Error opening folder %s", fs->path);
		AfxMessageBox(szMessage, MB_ICONSTOP|MB_OK);
	}
}

void CXferWindow::SendFile(XferFileSpec* fs)
{
	if(mXferQueue)
	{
		XferFileSpec*myfs = (XferFileSpec*) pgp_malloc(sizeof(XferFileSpec));
		
		memcpy(myfs, fs, sizeof(XferFileSpec));
		
		if(fs->root)
		{
			myfs->root = myfs->path + (fs->root - fs->path) ;
		}
		
		myfs->filename = myfs->path + (fs->filename - fs->path) ;
		
		mXferQueue->Send(_mt_sendFile, myfs, sizeof(XferFileSpec), 0);
	}
}

void CXferWindow::QueueSend(XSendFile *nxs)
{
	HICON smallIcon, largeIcon;
	
	GetFileIcon(nxs->xi.filepath, &smallIcon, &largeIcon);
	
	EnterCriticalSection(&mSendCriticalSection);
	
	mNumSends++;
	
	int iconIndex = mSendSmallImageList.Add(smallIcon);	
	int itemIndex = mSendList.GetItemCount();
	
	mSendList.InsertItem(itemIndex, nxs->xi.filename, iconIndex);
	
	DestroyIcon(smallIcon);
	DestroyIcon(largeIcon);
	
	ItemInfo* info = new ItemInfo;
	
	info->iconIndex = iconIndex;
	info->xsf = nxs;

	mSendList.SetItemData( itemIndex,(DWORD) info);
	
	LeaveCriticalSection(&mSendCriticalSection);
}

void CXferWindow::QueueReceive(XRcvFile *nxr)
{
	HICON smallIcon;
	
	ShowWindow(SW_SHOW);
	
	smallIcon = (HICON)LoadImage(	theApp.m_hInstance,
							MAKEINTRESOURCE(IDI_RECVICON),
							IMAGE_ICON, 
							16, 
							16,
							LR_SHARED);
		
	EnterCriticalSection(&mRecvCriticalSection);
	
	mNumReceives++;
	
	int iconIndex = mRecvSmallImageList.Add(smallIcon);
	int itemIndex = mRecvList.GetItemCount();
	
	mRecvList.InsertItem(itemIndex, nxr->xi.filename, iconIndex);
	
	ItemInfo* info = new ItemInfo;
	
	info->iconIndex = iconIndex;
	info->xrf = nxr;

	mRecvList.SetItemData( itemIndex,(DWORD) info); 
	
	LeaveCriticalSection(&mRecvCriticalSection);
}

void CXferWindow::DequeueSend(XSendFile *nxs)
{
	EnterCriticalSection(&mSendCriticalSection);
	
	mNumSends--;
	
	int count = mSendList.GetItemCount();
	
	ItemInfo* info;
	
	for(int index = 0; index < count; index++)
	{
		info = (ItemInfo*) mSendList.GetItemData(index);

		if( info->xsf == nxs )
		{
			mSendSmallImageList.Remove(info->iconIndex);
			
			delete info;
			
			mSendList.DeleteItem(index);
		}
	}
	
	LeaveCriticalSection(&mSendCriticalSection);
}

void CXferWindow::DequeueReceive(XRcvFile *nxr)
{
	EnterCriticalSection(&mRecvCriticalSection);
	
	mNumReceives--;
	
	int count = mRecvList.GetItemCount();
	
	ItemInfo* info;
	
	for(int index = 0; index < count; index++)
	{
		info = (ItemInfo*) mRecvList.GetItemData(index);

		if( info->xrf == nxr )
		{
			//mRecvSmallImageList.Remove(info->iconIndex);
			
			delete info;
			
			mRecvList.DeleteItem(index);
		}
	}
	
	LeaveCriticalSection(&mRecvCriticalSection);
}


void CXferWindow::StartSend(XferInfo *xi)
{
	char string[256];
	HICON hIcon;
	
	mSendXferInfo = xi;
	
	mSendFile.SetWindowText(xi->filename);
	
	wsprintf(string, "0%% of %d K", xi->bytesTotal/1024);
	mSendSize.SetWindowText(string);
	
	mSendProgress.SetPos(0);
	
	EnterCriticalSection(&mSendCriticalSection);
	
	mNumSends--;
	
	ItemInfo* info = (ItemInfo*) mSendList.GetItemData(0); 
	//mSendSmallImageList.Remove(info->iconIndex);
	mSendList.DeleteItem(0);
	
	delete info;
	
	LeaveCriticalSection(&mSendCriticalSection);
}

void CXferWindow::StartReceive(XferInfo *xi)
{
	char string[256];
	
	mRecvXferInfo = xi;
	
	mRecvFile.SetWindowText(xi->filename);
	
	wsprintf(string, "0%% of %d K", xi->bytesTotal/1024);
	mRecvSize.SetWindowText(string);
	
	mRecvProgress.SetPos(0);
	
	EnterCriticalSection(&mRecvCriticalSection);
	
	mNumReceives--;
	
	ItemInfo* info = (ItemInfo*) mRecvList.GetItemData(0); 
	//mRecvSmallImageList.Remove(info->iconIndex);
	mRecvList.DeleteItem(0);
	
	delete info;
	LeaveCriticalSection(&mRecvCriticalSection);
}

void CXferWindow::EndSend()
{
	EnterCriticalSection(&mSendCriticalSection);
	
	mSendXferInfo = NULL;
	
	mSendFile.SetWindowText(szNoSend);
	mSendSize.SetWindowText("");
	mSendProgress.SetPos(0);
	
	LeaveCriticalSection(&mSendCriticalSection);
}

void CXferWindow::EndReceive()
{
	EnterCriticalSection(&mRecvCriticalSection);
	
	mRecvXferInfo = NULL;
	mRecvFile.SetWindowText(szNoRecv);
	mRecvSize.SetWindowText("");
	mRecvProgress.SetPos(0);
	
	LeaveCriticalSection(&mRecvCriticalSection);
}


const GUID MY_CLSID_ShellLink = 
	{ 0x00021401L, 0, 0, { 0xC0,0,0,0,0,0,0,0x46 } };
const GUID MY_IID_IShellLink = 
	{ 0x000214EEL, 0, 0, { 0xC0,0,0,0,0,0,0,0x46 } };
                
HRESULT CXferWindow::ResolveLink(LPCSTR lpszLinkFile, LPSTR lpszPath) 
{ 
    HRESULT hres;     
    IShellLink* psl;     
    char szGotPath[MAX_PATH]; 
    char szDescription[MAX_PATH];     
    WIN32_FIND_DATA wfd;  
    *lpszPath = 0; // assume failure  
    
    CoInitialize(NULL);
    
    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(MY_CLSID_ShellLink, NULL, 
            CLSCTX_INPROC_SERVER, MY_IID_IShellLink, (void **)&psl); 
            
    if (SUCCEEDED(hres)) 
    {         
    	IPersistFile* ppf;  
    	
        // Get a pointer to the IPersistFile interface. 
        hres = psl->QueryInterface(	IID_IPersistFile, 
        							(void **)&ppf);    
             
        if (SUCCEEDED(hres)) 
        { 
        	WORD wsz[MAX_PATH];  
        	// Ensure that the string is Unicode.
        	// Load the shortcut.
        	MultiByteToWideChar(CP_ACP, 0, lpszLinkFile, -1, (wchar_t *)wsz, MAX_PATH);  
        	hres = ppf->Load((wchar_t *)wsz, STGM_READ); 
        
            if (SUCCEEDED(hres)) 
            {   
            	// Resolve the link. 
                hres = psl->Resolve(NULL, SLR_ANY_MATCH); 
                
                if (SUCCEEDED(hres)) 
                {  
                    // Get the path to the link target.
                    hres = psl->GetPath(szGotPath,
                        MAX_PATH, (WIN32_FIND_DATA *)&wfd,
                        SLGP_SHORTPATH);
                     
                    //if (!SUCCEEDED(hres)
                        //HandleErr(hres); // application-defined function
                        
                    // Get the description of the target.
                    hres = psl->GetDescription(szDescription, MAX_PATH);
                     
                    //if (!SUCCEEDED(hres))
                    	//HandleErr(hres); // application-defined function
                        
                    lstrcpy(lpszPath, szGotPath);
            	}
            }
            
        	// Release the pointer to the IPersistFile interface. 
    		ppf->Release();       
    	} 
    	
    	// Release the pointer to the IShellLink interface.
    	psl->Release();     
    
    }  
    
    CoUninitialize( );
    
    return hres; 
} 

	
BEGIN_MESSAGE_MAP(CXferWindow, CWnd)
	//{{AFX_MSG_MAP(CXferWindow)
	ON_WM_DROPFILES()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
