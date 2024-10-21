/*
History:
-------
Updated: 

.5/10/2003 (v1.1) to incorporate suggestions from Matitiahu Allouche:
[1] The program now displays an informative error message if the machine is not correct.
A file called "code_sig.sig" is created to this effect, this file contains the MD5 of the encrypted "code_sec.dat" in the first line and MD5 of the decrypted contents in the second.

About:
------
This is the main source to the ExeProtector program. This program will be with the software publisher who wishes to protect his programs
The HardwareID obtained from the user's machine will be entered and then the program file to be
"protected" will be selected. The code or text section of the file will
be separated into "code_sec.dat". The time taken to do this depends upon  the program filesize.
The rest of the program information alongwith the code section after the program entry point which is now
overwritten with binary NULLs are copied into "Real.EXE". The program also creates a file called "Patch.dat"
which contains the real size of code section before it was made 8 byte aligned to suit the current Blowfish code

This file "Real.exe" or whatever name you give it has an invalid code section and will NOT run by itself - it is loaded by Loader32.EXE on the customer's machine, and "code_sec.dat" is read and the decrypted
data is written into the program memory space. Only the correct hardware will form the correct decryption key thus running the program . In all other cases,
garbage data will be written into the process thus crashing it.

  The reason why "code_sec.dat" and "Patch.dat" had to be created was that I am currently using Blowfish to encrypt 
  the code section of the PE files. Now the Virtual size of the code section from the program entry point onwards 
  may NOT be a 8 byte block, so I have to pad it up with binary 0s. Hence, I just cannot put the encrypted code section
  into the PE file itself - a fact that I WOULD LOVE TO.
  Now as the code section was padded up, we really need to know how much of the decrypted data (which is thus multiple of 8 bytes)
  we should read, and not accindentally read the padding !

If anyone can recommend an equally good algo with same or better chipher strength as Blowfish - or any Blowfish implementation which can operate on
a single char at a time (I raelly don't think that's possible as Blowfish is a 64Bit block algo - 8 byte blocks!), but the Blowfish reference code at
CounterPane had offered me some hope - it just does not work , and there's no source too to work on!

  ANY SUGGESTIONS ?
/////////////////////////////////
"Climb every mountain ...
 ... until you find your dream."
  - The Sound of Music
/////////////////////////////////
*/
#include "stdafx.h"
#include "ExeProt.h"
#include "ExeProtDlg.h"
///////////////////////////////////////////////////////
#include "MD5.h"
#pragma comment(lib,"MD5.lib")//Explicit linkage

#include <imagehlp.h>//include Imagehlp.lib into project
#pragma comment(lib,"imagehlp.lib")
///////////////////////////////////////////////////////
#include <fstream.h>
#include "BlowFish.h"
DWORD RVAToFileOffset(BOOL ,PIMAGE_NT_HEADERS,DWORD ,unsigned );
BOOL DumpSection(DWORD ,DWORD ,DWORD );
BOOL BF_Encrypt(char *szInputFile,const char *szKey);
DWORD GetOutputLength(DWORD lInputLong);
char szMyFileName[MAX_PATH+1];
///////////////////////////////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExeProtDlg dialog

CExeProtDlg::CExeProtDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExeProtDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExeProtDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CExeProtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExeProtDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CExeProtDlg, CDialog)
	//{{AFX_MSG_MAP(CExeProtDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnDumpAndEncrypt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExeProtDlg message handlers

BOOL CExeProtDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CExeProtDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CExeProtDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CExeProtDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


void CExeProtDlg::OnDumpAndEncrypt() 
{
	CString strLoadEXE,strEXEPath,sKey;
	GetDlgItemText(IDC_EDIT1,sKey);
	if(sKey=="") 
	{
		MessageBox("Please enter the Machine ID MD5 key of the customer !","Error reading MachineID",MB_ICONHAND|MB_OK);
		return ; //No key entered
	}
	
	CFileDialog dlg(TRUE,NULL,strLoadEXE,OFN_FILEMUSTEXIST,"Win32 Executable file(*.EXE)|*.EXE||",this);
	if(dlg.DoModal()==IDOK) 
	{
		strLoadEXE=dlg.GetFileName();
		strEXEPath=dlg.GetPathName();
	}
	else return; //User cancelled
	
	LOADED_IMAGE li;
	lstrcpy(szMyFileName,(LPCTSTR)strEXEPath);
	if (MapAndLoad(szMyFileName,0,&li,FALSE,TRUE)) RVAToFileOffset(TRUE,li.FileHeader,li.FileHeader->OptionalHeader.AddressOfEntryPoint,li.FileHeader->FileHeader.NumberOfSections);
    else
    {
        AfxMessageBox("MapAndLoad failed !");
        return;
    }
	UnMapAndLoad( &li );
	BF_Encrypt("code_sec.dat",(LPCSTR)sKey);
	
}

DWORD RVAToFileOffset(BOOL ,PIMAGE_NT_HEADERS pNtHdr,DWORD rva,unsigned nNumberOfSections)
 {
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HANDLE      hFileMapping=NULL; // Handle of memory mapped file
    PVOID       pMemoryMappedFileBase;
     
	hFile=CreateFile(szMyFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,(HANDLE)0);
	hFileMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
    pMemoryMappedFileBase=(PCHAR)MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,0);
	
     if (pMemoryMappedFileBase==0)
     {
         //Error
         CloseHandle(hFileMapping);
         hFileMapping=0;
         CloseHandle(hFile);
         hFile=INVALID_HANDLE_VALUE;
         return ((DWORD)-1);
	 }
     
    PIMAGE_SECTION_HEADER pSectHdr=IMAGE_FIRST_SECTION(pNtHdr);
	DWORD cbMaxOnDisk=0,startSectRVA=0,endSectRVA=0,dwResult=(DWORD)-1;
     
     for(unsigned i=0;i<nNumberOfSections;i++,pSectHdr++)
     {
         cbMaxOnDisk=min(pSectHdr->Misc.VirtualSize,pSectHdr->SizeOfRawData);
 
         startSectRVA = pSectHdr->VirtualAddress;
         endSectRVA = startSectRVA + cbMaxOnDisk;
         
         if((rva>=startSectRVA)&&(rva<endSectRVA)) dwResult=pSectHdr->PointerToRawData+(rva-startSectRVA);
     }
     	
	CloseHandle(hFileMapping);
    CloseHandle(hFile);
	if(dwResult<0) MessageBox(NULL,"Error calculating file offset (what type of PE file is this ?)","Error",MB_OK|MB_ICONQUESTION);
	unsigned int nNumberOfBytesToRead=((pSectHdr-nNumberOfSections)->Misc.VirtualSize-(rva-(pSectHdr-nNumberOfSections)->VirtualAddress));//We will be reading (rva-start of section) bytes after start of .text offset. We are reading VirtualSize bytes of data because that's really the size of actual code before padding it up by the linker
	 
	if(DumpSection(dwResult,nNumberOfBytesToRead,rva)) MessageBox(NULL,"The code section of the file has been dump sucessfully","Done !",MB_OK|MB_ICONINFORMATION);;
     return dwResult;
}

BOOL DumpSection(DWORD nFileOffset,DWORD dwNumberOfBytesToRW,DWORD dwRVA)
	{
	 ifstream file;
	 ofstream dumpSec("code_sec.dat",ios::binary);
	 file.open(szMyFileName,ios::binary);
	 unsigned char c=NULL;//Hex bytes,remember ? (0 to 255 -> 0 to ff)
	 //Get number of bytes to read from .text or code section
	 //We want the .text or code section which is the first section
	 
	 file.seekg(nFileOffset,ios::beg);
	 for(int i=0;i<dwNumberOfBytesToRW;i++){
		 file.read((char *)&c,1);
		 dumpSec.write((char *)&c,1);
	 }
	 dumpSec.close();
	 unsigned int nResumeCur=file.tellg();
	 file.close();
	 file.open(szMyFileName,ios::binary);
	 dumpSec.open("real.exe",ios::binary);
	 while(dumpSec&&file)
	 {
 		 if(file.tellg()==nFileOffset) //Are we going to read the entry point next?
		 {
			 //Write NULLs over the program starting from program entry point
			 c=0;
			 for(i=0;i<dwNumberOfBytesToRW;i++) dumpSec.write((char *)&c,1);
			 
			 file.seekg(nResumeCur);
		 }
		 file.read((char *)&c,1);
		 dumpSec.write((char *)&c,1);
	 }
	 dumpSec.close();
	 file.close();
	 return TRUE;
	}

BOOL BF_Encrypt(char *szInputFile,const char* szKey)
{
	HANDLE hFile=CreateFile(szInputFile,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;
	DWORD dwSize=GetFileSize(hFile,NULL);
	CloseHandle(hFile);
	ifstream readFile(szInputFile,ios::binary);
	if(!readFile) {MessageBox(NULL,"Unable to open source file.","Debug",MB_OK);return -1;}
	int encRet;
	char *readBuffer,*outBuffer;
		
	//Create a dynamic array
	readBuffer=new char[dwSize];
	outBuffer=new char[GetOutputLength(dwSize)];//What's the outputBuffer length ?
	/////////////////////////////////////////////////////////////////////////////////////////
	//Write actual number of bytes (without padding)
	ofstream f("Patch.dat");
	f<<dwSize;
	f.close();
	/////////////////////////////////////////////////////////////////////////////////////////
	BlowFishEnc encryption(szKey);

	readFile.read((char *)readBuffer,dwSize); //This single line reads in whole of "code_sec.dat"
	readFile.close();
	/////////////////////////////////////////////////////////////////////////////////////////
	ofstream writeFile(szInputFile,ios::binary);
	if(!writeFile) {MessageBox(NULL,"Unable to open destination file.","Debug",MB_OK);return -1;}
	encRet=encryption.encryptStream(readBuffer,dwSize,outBuffer);
	writeFile.write((char *)outBuffer,encRet);
	writeFile.close();
	/*
	We just read in the plain data, encrypted it to a buffer and overwrote the plain data with the cipher
	*/
	/////////////////////////////////////////////////////////////////////////////////////////
	//Let's do a MD5 on readBuffer(plaintext), outbuffer(cipher) and write it out to "code_sig.sig"
	f.open("code_sig.sig");
	f<<MD5String((unsigned char*)outBuffer,GetOutputLength(dwSize))<<"\n";
	f<<MD5String((unsigned char*)readBuffer,dwSize);
	f.close();
	/////////////////////////////////////////////////////////////////////////////////////////
	delete[] readBuffer;
	delete[] outBuffer;
	return TRUE;
}
//Stolen from BlowFish.cpp ;)
DWORD GetOutputLength(DWORD lInputLong)
{
	DWORD lVal=lInputLong%8;// find out if uneven number of bytes at the end
	if(lVal!=0) return (lInputLong + 8 - lVal);
	return lInputLong;
}