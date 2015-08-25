/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: PGPFone.h,v 1.7 1999/03/10 02:50:53 heller Exp $
____________________________________________________________________________*/
#ifndef PGPFONE_H
#define PGPFONE_H

#define PGPFONEOPTSVERSION	18
#define PHONENUMBERLEN		80
#define MAXDIALSETUPS		16
#define PORTNAMELEN			32
#define BAUDRATELEN			8
#define MODEMINITLEN		80

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

#include "pgpLeaks.h"
#include "pgpMem.h"

#ifdef PGP_WIN32
#include "StdAfx.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif	// _DEBUG
#define PRIORITY

#endif	// PGP_WIN32

#if	PGP_MACINTOSH

#define pgp_errstring(x)	SignalCStr_(x)

#define LONG_TO_BUFFER(l, b)		\
	*(ulong*)(b) = (l)
#define BUFFER_TO_LONG(b, l)		\
	(l) = *(ulong*)(b)
#define SHORT_TO_BUFFER(s, b)		\
	*(ushort*)(b) = (s)
#define BUFFER_TO_SHORT(b, s)		\
	(s) = *(ushort*)(b)

#elif	PGP_WIN32

#define pgp_errstring(x)										\
{																\
	AfxMessageBox(x, MB_OK|MB_TASKMODAL|MB_ICONEXCLAMATION|		\
		MB_SETFOREGROUND);										\
	pgpAssert(0);												\
}
#define Boolean				short

#define LONG_TO_BUFFER(l, b)		\
{									\
	((uchar*)(b))[0] = (uchar)((l) >> 24);	\
	((uchar*)(b))[1] = (uchar)((l) >> 16);	\
	((uchar*)(b))[2] = (uchar)((l) >>  8);	\
	((uchar*)(b))[3] = (uchar)((l));		\
}
#define BUFFER_TO_LONG(b, l)					\
	(l) = (((ulong)((uchar*)(b))[0]) << 24) |	\
		  (((ulong)((uchar*)(b))[1]) << 16) |	\
		  (((ulong)((uchar*)(b))[2]) <<  8) |	\
		  (((ulong)((uchar*)(b))[3]))
#define SHORT_TO_BUFFER(s, b)				\
{											\
	((uchar*)(b))[0] = (uchar)((s) >> 8);	\
	((uchar*)(b))[1] = (uchar)((s));		\
}
#define BUFFER_TO_SHORT(b, s)					\
	(s) = (((ushort)((uchar*)(b))[0]) << 8) |	\
		  (((ushort)((uchar*)(b))[1]))

#endif	// PGP_WIN32

typedef short PGErr;
enum PGPFoneErrors	{	_pge_NoErr = 0,
						_pge_PortNotAvail,
						_pge_ModemNotAvail,
						_pge_InternalAbort,
						_pge_NoSrvrName
					};


enum ContactMethods	{	_cme_Serial,
						_cme_Internet,
						_cme_AppleTalk
					};

#ifdef	MACINTOSH
typedef struct SerialOptions
{
	uchar	portName[PORTNAMELEN];
	char	modemInit[MODEMINITLEN];
	char	baud[BAUDRATELEN];
	uchar	frameType;				// Bits 0-5: like bits 10-15 in serConfig word on IM II, p250
	uchar	handshaking;			// Bit 0:SoftIn, 1:SoftOut, 2:HardIn, 3:HardOut
	ushort	dcdStyle:3, dtrHangup:1;// how to use carrier detect, whether to use DTR hangup
	short	maxBaud;				// 0:9600 1:14400 2:28800
} SerialOptions;

typedef struct SystemOptions
{
	Rect	pgfWindowBounds, authWindowBounds;
	short	pgfWindowState;
} SystemOptions;

typedef struct FileXferOptions
{
	// default receive folder
	uchar volName[32];
	long dirID;
	long textFileCreator;
	uchar creatorAppName[32];
} FileXferOptions;

#elif	PGP_WIN32
typedef struct SerialOptions
{
	char	modemInit[MODEMINITLEN];
	ulong	baud;
	char	port[5];
	short	dtrHangup;
	short	maxBaud;
} SerialOptions;

typedef struct SystemOptions
{
	int	windowPosX;
	int	windowPosY;
	int viewStatusInfo;
	int viewEncodingDetails;
			
} SystemOptions;

typedef struct FileXferOptions
{
	// default receive folder
	char recvDir[MAX_PATH + 1];
} FileXferOptions;

#endif	//	PGP_WIN32

typedef struct PhoneOptions
{
	char	identity[32];	// user's cleartext identifying name
	short	prefEncryptor;
	ushort	cryptorMask;	// 1:CAST,2:3DES,3:Blowfish,4:None
	ulong	prefCoder;
	ulong	altCoder;
	short	prefPrime;		// 0=512...2=1024...4=2048
	ushort	primeMask;		// 1:512,2:768,3:1024,4:1536,5:2048,6:3072,7:4096
	ushort	alwaysListen : 1,
			playRing : 1,
			fullDuplex : 1,
			idIncoming : 1,
			idOutgoing : 1,
			idUnencrypted : 1;
	enum ContactMethods connection;
	ushort	siGain;			// Sound input gain 0-255
	short	siThreshold;	// Sound input minimum amplitude 0-32767
} PhoneOptions;

typedef struct LicenseOptions
{
	ulong	installDate;
	char	ownerName[64];
	char	ownerCompany[64];
	char	ownerLicense[64];
} LicenseOptions;

typedef struct ContactEntry
{
	char				name[64];
	enum ContactMethods	method;
	//	Serial
	char				phoneNumber[PHONENUMBERLEN];
	char				modemInit[MODEMINITLEN];
	short				maxRedials;		// or 0 for no maximum
	short				useDialInfo,
						nonLocal;
	//	Internet
	ushort				useIPSearch : 1;
	char				internetAddress[128];
	ulong				ipSearchStart,
						ipSearchEnd;
	//	AppleTalk
	char				nbpZone[34];
	char				nbpListener[34];
} ContactEntry;

typedef struct LocalDialInfo
{
	char	localityName[32];
	char	areaCode[4];
	char	catBefore[3][32];	//0:local  1:nonlocal  2:longdist
	char	catAfter[3][32];	//0:local  1:nonlocal  2:longdist
	long	redialDelay;		// ticks between redials
	ushort	usePulseDial:1;
} LocalDialInfo;

typedef struct DialingOpts
{
	short	numDialSetups, curDialSet;
	LocalDialInfo dialInfo[MAXDIALSETUPS];
} DialingOpts;

typedef struct PGPFoneOptions
{
	ushort 			version;
	PhoneOptions	popt;
	SerialOptions	sopt;
	DialingOpts		dopt;
	FileXferOptions fopt;
	SystemOptions	sysopt;
	LicenseOptions	licopt;
} PGPFoneOptions;

extern PGPFoneOptions gPGFOpts;

#endif

