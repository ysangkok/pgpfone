/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: PGPFoneUtils.cpp,v 1.3.10.1 1999/07/09 22:42:38 heller Exp $
____________________________________________________________________________*/
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef	PGP_MACINTOSH
#include <UEnvironment.h>
#include <OpenTransport.h>
#include <Sound.h>
#endif

#include "PGPFoneUtils.h"

#include "CEncryptionStream.h"
#include "CRC.h"
#include "LMutexSemaphore.h"


/*	The SafeAlloc defs and funcs below establish
	an interrupt safe method of allocating memory.
	It also allows us to bypass system memory
	allocation which can take a significant amount
	of time during real-time operations like those
	in PGPFone.
	
	We only allow allocation of one size
	that fits all blocks for simplicity.
*/
#ifdef	PGPXFER
#define NUMSAFEALLOCS	160L
#else
#define NUMSAFEALLOCS	64L
#endif
#define SAFEALLOCSIZE	2176L	// 2048 + 128

typedef struct SafeAllocEntry
{
	void		*data;
	short		inUse;			// can be greater than 1! == reference count
} SafeAllocEntry;

char			*safeAllocStorage;
SafeAllocEntry	safeAllocs[NUMSAFEALLOCS];
short			safesAllocated,
				maxSafes;
LMutexSemaphore	*safeMutex,
				debugMutex;
#ifdef PGP_WIN32
static HANDLE	heapID = 0;
#endif

void InitSafeAlloc()
{
	safeAllocStorage = (char *)pgp_malloc(SAFEALLOCSIZE * NUMSAFEALLOCS);
	for(short inx = 0; inx < NUMSAFEALLOCS ; inx++)
	{
		safeAllocs[inx].data = safeAllocStorage + SAFEALLOCSIZE * inx;
		safeAllocs[inx].inUse = 0;
	}
	safesAllocated = 0;
	maxSafes = 0;
	safeMutex = new LMutexSemaphore();
}

void DisposeSafeAlloc()
{
	delete safeMutex;
	pgpAssert(!safesAllocated);
	memset(safeAllocs[0].data, 0, SAFEALLOCSIZE * NUMSAFEALLOCS);
	pgp_free(safeAllocStorage);
#ifdef BETA
	//DebugLog("Max Safes: %d", maxSafes);
#endif
}

void *safe_malloc(long len)
{
	void	*data=NIL;
	short	inx;
	
	(void) len;
	
	pgpAssert(len <= SAFEALLOCSIZE);
	safeMutex->Wait();
	for(inx=0;inx<NUMSAFEALLOCS;inx++)
	{
		if(!safeAllocs[inx].inUse)
		{
			data = safeAllocs[inx].data;
			safeAllocs[inx].inUse = 1;
			safesAllocated++;
			if(safesAllocated > maxSafes)
				maxSafes = safesAllocated;
			//DebugLog("safemalloc: %d, %ld", inx, len);
			break;
		}
	}
	if(!data)
		pgp_errstring("No free safes!");
	safeMutex->Signal();
	return data;
}

void safe_free(void *data)
{
	short inx = ((char *)data - (char *)safeAllocs[0].data) / SAFEALLOCSIZE;
	
	safeMutex->Wait();
	pgpAssert(safeAllocs[inx].inUse > 0);	//double free error
	pgpAssert((((char *)data - (char *)safeAllocs[0].data) %
				SAFEALLOCSIZE) == 0);		//unknown block error
	safeAllocs[inx].inUse --;
	if(!safeAllocs[inx].inUse)
		safesAllocated--;
	//DebugLog("safe_free: %d", inx);
	safeMutex->Signal();
}

void safe_increaseCount(void *data)
{
	short inx = ((char *)data - (char *)safeAllocs[0].data) / SAFEALLOCSIZE;
	
	safeMutex->Wait();
	pgpAssert(safeAllocs[inx].inUse > 0);	// nobody holds this block!
	pgpAssert((((char *)data - (char *)safeAllocs[0].data) %
				SAFEALLOCSIZE) == 0);		//unknown block error
	safeAllocs[inx].inUse ++;
	//DebugLog("safe_increaseCount: %d", inx);
	safeMutex->Signal();
}

ulong pgp_getticks()
{
	// Returns less accurate milliseconds since system startup
#ifdef PGP_MACINTOSH
	return LMGetTicks() * 50 / 3;
#elif PGP_WIN32
	return GetTickCount();
#endif
}

ulong pgp_milliseconds()
{
	// Returns milliseconds since system startup
#ifdef PGP_MACINTOSH
	if(UEnvironment::HasFeature(env_HasOpenTransport))
	{
		OTTimeStamp soundStamp;
		
		OTGetTimeStamp(&soundStamp);
		return OTTimeStampInMilliseconds(&soundStamp);
	}
	else
		return pgp_getticks();
#elif PGP_WIN32
	return GetTickCount();
#endif
}

#ifdef PGP_WIN32

void pgp_meminit(void)
{
	heapID = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 8192, 0);
}

uchar pgp_memvalidate(void *ptr)
{
	return HeapValidate(heapID, 0, ptr);
}

void pgp_memdone(void)
{
	HeapDestroy(heapID);
}

#endif

void *pgp_malloc(long len)
{
	void	*ptr;

	//DebugLog("malloc:%ld\r",len);
#ifdef	PGP_MACINTOSH
	ptr = NewPtr(len);
#elif	PGP_WIN32
	ptr = HeapAlloc(heapID, HEAP_ZERO_MEMORY|HEAP_GENERATE_EXCEPTIONS, len);
#endif
	pgpAssert(IsntNull(ptr));
	return ptr;
}

void *pgp_mallocclear(long len)
{
#ifdef	PGP_MACINTOSH
	Ptr temp;

	/* allocate a block of desiredLen and clear it to 0 */
	temp = NewPtrClear(len);
	pgpAssert(IsntNull(temp));
	return temp;
#elif	PGP_WIN32
	void	*ptr = pgp_malloc(len);

	memset(ptr, 0, len);
	return ptr;
#endif
}

void pgp_memcpy(void *dest, void *src, long len)
{
#ifdef	PGP_MACINTOSH
	BlockMoveData(src, dest, len);
#elif	PGP_WIN32
	memcpy(dest, src, len);
#endif
}

void *pgp_realloc(void *orig, long newLen)
{
#ifdef	PGP_MACINTOSH
	Ptr newSpace;
	long oldLen;
	
	if(orig)
		SetPtrSize((Ptr)orig, newLen);
	else
		orig=NewPtr(newLen);
	if(MemError())
	{
		if((newSpace = NewPtr(newLen)) != NULL)
		{
			oldLen=GetPtrSize((Ptr)orig);
			BlockMove(orig, newSpace, oldLen);
			DisposePtr((Ptr)orig);
			orig=newSpace;
		}
		else
			orig = NIL;
	}
	return orig;
#elif	PGP_WIN32
	void	*ptr = NULL;
#ifdef _DEBUG
	if (!HeapValidate(heapID, 0, NULL))
		DebugLog("validation failed before reallocating %d bytes at %p",
			newLen, ptr);
#endif	// _DEBUG

	ptr = HeapReAlloc(heapID, HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,
		ptr, newLen);

#ifdef _DEBUG
	if (!HeapValidate(heapID, 0, NULL))
		DebugLog("validation failed after reallocating %d bytes at %p",
			newLen, ptr);
#endif	// _DEBUG

	return ptr;
#endif	// PGP_WIN32
}

void pgp_free(void *freePtr)
{
#ifdef	PGP_MACINTOSH
	/*long len = GetPtrSize((Ptr)freePtr);
	DebugLog("free:%ld\r",GetPtrSize((Ptr)freePtr));*/
	DisposePtr((Ptr)freePtr);
#elif	PGP_WIN32
#ifdef _DEBUG
	if (!HeapValidate(heapID, 0, NULL))
		DebugLog("validation failed before freeing %p", freePtr);
#endif	// _DEBUG

	HeapFree(heapID, 0, freePtr);

#ifdef _DEBUG
	if (!HeapValidate(heapID, 0, NULL))
		DebugLog("validation failed after freeing %p", freePtr);
#endif	// _DEBUG
#endif	// PGP_WIN32
}

//	show a dialog box with message inCStr, if allowCancel is set,
//	a cancel button will be available.  Returns 0 for cancel,
//	and 1 for OK.

short PGFAlert(char *inCStr, short allowCancel)
{
#ifdef	PGP_MACINTOSH
	DialogPtr pfAlert;
	GrafPtr savePort;
	short result=0, i;
	Str255 s;
	
	GetPort(&savePort);
	strcpy((char *)s, inCStr);
	c2pstr((char *)s);
	if((pfAlert = GetNewDialog(130,nil,(WindowPtr)-1L)) != NULL)
	{
		SetPort(pfAlert);
		ParamText((uchar *)s,"\p","\p","\p");
		if(!allowCancel)
			HideDialogItem(pfAlert,2);
		SetDialogCancelItem(pfAlert, 2);
		SetDialogDefaultItem(pfAlert, 1);
		ShowWindow(pfAlert);
		SysBeep(30);
		ModalDialog(nil, &i);
		DisposeDialog(pfAlert);
		result = (i==1);
	}
	SetPort(savePort);
	return result;
#elif	PGP_WIN32
	if(!allowCancel)
		AfxMessageBox(inCStr, MB_OK|MB_TASKMODAL|MB_ICONEXCLAMATION|
			MB_SETFOREGROUND);
	else
		AfxMessageBox(inCStr, MB_OKCANCEL|MB_TASKMODAL|
			MB_ICONEXCLAMATION|MB_SETFOREGROUND);
	return 0;
#endif
}

void DebugLogData(void *s, long len)
{
#ifdef	PGP_MACINTOSH
	short ref;

	if(FSOpen("\pDebugLog",0,&ref))
	{
		Create("\pDebugLog",0,'CWIE','TEXT');
		FSOpen("\pDebugLog",0,&ref);
	}
	SetFPos(ref,fsFromLEOF,0);
	FSWrite(ref,&len,s);
	FSClose(ref);
#elif	PGP_WIN32
	FILE	*f;
	static Boolean init = TRUE;
	char attrib[5] = "ab";
	
	if(init)
	{
		init = FALSE;
		strcpy(attrib, "wb+");
	}
	
	if (! (f = (FILE*)fopen("C:\\DebugLog.txt", attrib)))
		return;
	fwrite(s, 1, len, f);
	fclose(f);
#endif
}

void DebugLog(char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	long len;
	
	debugMutex.Wait();
	va_start(ap, fmt);
	len = vsprintf(buf, fmt, ap);
	va_end(ap);
	strcat(buf,"\r\n");
	len+=2;
	DebugLogData(buf,len);
	debugMutex.Signal();
}

void num2strpad(uchar *dest, register long num, short len, short max, char padChar)
{
	uchar s[32];
	register uchar *p;
	short neg=0, digit=0;
	
	p=s+31;
	*p=0;
	if(num<0)
	{
		neg=1;
		num=-num;
	}
	digit=0;
	do
	{
		if(max<0)
		{
			if(digit>0 && digit%3==0)
				*--p=',';
			digit++;
		}
		*--p='0'+(num%10);
		num/=10;
	} while(num>0);
	if(neg)
		*--p='-';
	len=(p-s)-(31-len);
	while(len>0)
	{
		*--p=padChar;
		len--;
	}
	if(max<0)
		max=-max;
	strncpy((char *)dest, (char *)p, max);
}

void num2str(uchar *dest, long num, short len, short max)
{
	num2strpad(dest, num, len, max, ' ');
}

long minl(long x, long y)
{
	if(x<=y)
		return x;
	else
		return y;
}

long maxl(long x, long y)
{
	if(x>=y)
		return x;
	else
		return y;
}

short mins(short x, short y)
{
	if(x<=y)
		return x;
	else
		return y;
}

short maxs(short x, short y)
{
	if(x>=y)
		return x;
	else
		return y;
}

short maxi(short x, short y)
{
	if(x>=y)
		return x;
	else
		return y;
}

uchar CalcChecksum8(void *data, long len)
{
	char *dp=(char *)data;
	uchar check = 0;
	
	while(len--)
		check+=*dp++;
	return check;
}

ushort CalcCRC16(void *data, long len)
{
	ushort crc = 0;
	uchar *p, *e;
	
	p=(uchar *)data;
	e = p + len;
	while(p<e)
		crc = updatecrc16(*p++, crc);
	return crc;
}

ulong CalcCRC32(void *data, long len)
{
	ulong crc = 0xFFFFFFFF;
	uchar *p, *e;
	
	p=(uchar *)data;
	e = p + len;
	while(p<e)
		crc = updatecrc32(*p++, crc);
	return crc;
}

