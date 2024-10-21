// ExeProtDlg.h : header file
//

#if !defined(AFX_EXEPROTDLG_H__7F9D99D5_00FC_4A9B_904C_75645E015379__INCLUDED_)
#define AFX_EXEPROTDLG_H__7F9D99D5_00FC_4A9B_904C_75645E015379__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CExeProtDlg dialog

class CExeProtDlg : public CDialog
{
// Construction
public:
	CExeProtDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CExeProtDlg)
	enum { IDD = IDD_EXEPROT_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExeProtDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CExeProtDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDumpAndEncrypt();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXEPROTDLG_H__7F9D99D5_00FC_4A9B_904C_75645E015379__INCLUDED_)
