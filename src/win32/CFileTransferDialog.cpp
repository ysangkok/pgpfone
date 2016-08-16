/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CFileTransferDialog.cpp,v 1.5 1999/03/10 02:33:19 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"

#define INITGUID
#include <shlobj.h>
#undef INITGUID

#include "CPGPFone.h"

#include "CFileTransferDialog.h"

#include "CEncryptionStream.h"
#include "PGPFone.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef DEBUG
#undef new
#endif	// DEBUG
IMPLEMENT_DYNCREATE(CFileTransferDialog, CPropertyPage)
#ifdef DEBUG
#define new DEBUG_NEW
#endif	// DEBUG


static LPITEMIDLIST GetNextItemID(LPITEMIDLIST pidl);
static LPITEMIDLIST CopyItemID(LPITEMIDLIST pidl);
static void PrintStrRet(LPITEMIDLIST pidl, LPSTRRET lpStr, LPSTR name);
static int BrowseForFolder( LPSTR inpath, LPSTR outpath );

CFileTransferDialog::CFileTransferDialog()
	: CPropertyPage(CFileTransferDialog::IDD)
{
	//{{AFX_DATA_INIT(CFileTransferDialog)
	
	//}}AFX_DATA_INIT
}

CFileTransferDialog::~CFileTransferDialog()
{
}

void CFileTransferDialog::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileTransferDialog)
	DDX_Text(pDX, IDC_RECEIVE_FOLDER, mReceiveFolderString);
	//}}AFX_DATA_MAP
}

void
CFileTransferDialog::SetParameters(PGPFoneOptions *options)
{
	mReceiveFolderString = options->fopt.recvDir;
}

void
CFileTransferDialog::GetParameters(PGPFoneOptions *options)
{
	strcpy(options->fopt.recvDir,(LPCTSTR)mReceiveFolderString);
}

BEGIN_MESSAGE_MAP(CFileTransferDialog, CPropertyPage)
	//{{AFX_MSG_MAP(CFileTransferDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CFileTransferDialog::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	UpdateData(FALSE);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CFileTransferDialog::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch(LOWORD(wParam))
	{
		case IDC_BROWSE:
		{
			char currentpath[MAX_PATH];
			char newpath[MAX_PATH];
			
			strcpy(currentpath,(LPCTSTR)mReceiveFolderString);
			if( !BrowseForFolder( currentpath, newpath ))
			{
				mReceiveFolderString = newpath;
			
				UpdateData(FALSE);
			}
			break;
		}
	}
	
	return CPropertyPage::OnCommand(wParam, lParam);
}

LPMALLOC pMalloc;

const GUID MY_IID_ShellFolder = 
	{ 0x000214E6L, 0, 0, { 0xC0,0,0,0,0,0,0,0x46 } };

static int BrowseForFolder( LPSTR inpath,LPSTR outpath ) 
{
	LPSHELLFOLDER pFolder;
	BROWSEINFO bi;
	LPITEMIDLIST pidlBrowse;
	LPSTR szDisplayName;
	char path[MAX_PATH] = {0x00};
	char name[256] = {0x00};
	char* cp;
	char prompt[] = "Locate the folder you wish to use for "
					"receiving downloaded files:";        
	
	//get allocator
	if( !SUCCEEDED(SHGetMalloc( &pMalloc )) )
	{
		return 1;
	}
	
	szDisplayName = (LPSTR)pMalloc->Alloc(MAX_PATH);
	
	bi.hwndOwner 		= NULL;
	bi.pidlRoot  		= NULL;
	bi.pszDisplayName	= path;     
	bi.lpszTitle		= prompt;
	bi.ulFlags 			= BIF_RETURNONLYFSDIRS;
	bi.lpfn				= NULL;        
	bi.lParam			= 0;           
	bi.iImage			= 0;              

	pMalloc->Free(szDisplayName);
	
	pidlBrowse = SHBrowseForFolder(&bi);
	
	if(pidlBrowse != NULL)
	{
		if( SUCCEEDED(SHGetDesktopFolder(&pFolder )) )
		{
			LPITEMIDLIST pidl;
			
			// process each item
			for(pidl = pidlBrowse; 
				pidl != NULL; 
				pidl = GetNextItemID(pidl))
			{
				STRRET sName;
				LPSHELLFOLDER pSubFolder;
				LPITEMIDLIST pidlCopy;
				
				// copy item identifier
				if( (pidlCopy = CopyItemID(pidl)) == NULL)
				{
					break;
				}
				
				//Display the name
				if( SUCCEEDED(pFolder->GetDisplayNameOf(
						pidlCopy, SHGDN_INFOLDER, &sName) ) )
				{
					PrintStrRet(pidlCopy, &sName, name);
					
					strcat(path, "\\");
					strcat(path, name);
				}
				
				
				//Bind to the subfolder
				if( !SUCCEEDED(pFolder->BindToObject(
						pidlCopy, NULL, MY_IID_ShellFolder,
						(void **)&pSubFolder) ) )
				{
					pMalloc->Free(pidlCopy);
					break;
				}
				
				pMalloc->Free(pidlCopy);
				
				//Release the parent
				pFolder->Release();
				pFolder = pSubFolder;
			}
			
			// Release the last folder that was bound to
			if(pFolder != NULL)
			{
				pFolder->Release();
			}
		}
		
		// Free the PIDL for the folder they found
		pMalloc->Free(pidlBrowse);
	}
	
	//release Shell Allocator
	pMalloc->Release();
	
	cp = strchr(path, ':'); // colon in drive indicator
	
	if(cp)
	{
		cp--; //drive letter
	
		memmove(cp + 1, cp, 2);

		strcpy(outpath, cp + 1);
	}
	else
	{
		return 1;
	}
	
	return 0;
}


static LPITEMIDLIST GetNextItemID(LPITEMIDLIST pidl)
{
	int cb = pidl->mkid.cb;
	
	if(cb == 0)
		return NULL;
	
	pidl = (LPITEMIDLIST)(((LPBYTE)pidl) + cb);
	
	return ((pidl->mkid.cb == 0) ? NULL : pidl);
}

static LPITEMIDLIST CopyItemID(LPITEMIDLIST pidl)
{
	int cb = pidl->mkid.cb;
	
	LPITEMIDLIST pidlNew = (LPITEMIDLIST)
		pMalloc->Alloc(cb + sizeof(USHORT));
		
	if(pidlNew == NULL)
	{
		return NULL;
	}
	
	CopyMemory(pidlNew, pidl, cb);
	
	*((USHORT*) (((LPBYTE)pidlNew) + cb)) = 0;
	
	return pidlNew;
}

static void PrintStrRet(LPITEMIDLIST pidl, LPSTRRET lpStr, LPSTR name)
{
	LPSTR lpsz = NULL;
	int cch;
	
	switch(lpStr->uType)
	{
		case STRRET_WSTR:
		{
			cch = WideCharToMultiByte(	CP_ACP , 
										0, 
										lpStr->(u.pOleStr), 
										-1,
										lpsz, 
										0, 
										NULL, 
										NULL);
		
			lpsz = (LPSTR) pMalloc->Alloc(cch);
										
			if(lpsz != NULL)
			{
				WideCharToMultiByte(	CP_ACP , 
										0, 
										lpStr->(u.pOleStr), 
										-1,
										lpsz, 
										cch, 
										NULL, 
										NULL);
										
				strcpy(name, lpsz);
				
				pMalloc->Free(lpsz);
			}
			
			break;
		}
		
		case STRRET_OFFSET:
		{
			strcpy(name, ((char*)pidl)+lpStr->u.uOffset);
			break;
		}
		
		case STRRET_CSTR:
		{
			strcpy(name, lpStr->u.cStr);
			break;
		}	
	}
}
