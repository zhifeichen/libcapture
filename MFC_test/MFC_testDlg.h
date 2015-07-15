
// MFC_testDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CMFC_testDlg 对话框
class CMFC_testDlg : public CDialogEx
{
// 构造
public:
	CMFC_testDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MFC_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	void do_newuser(MyMSG* msg);
	void do_socket_error(MyMSG* msg);
	void do_userlogout(MyMSG* msg);
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	// show window
	CStatic m_winShow;
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();

	static void OnMsg(MyMSG* msg, void* userdata);
	CStatic m_winShowRemote;
};
