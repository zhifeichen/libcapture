
// MFC_testDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFC_test.h"
#include "MFC_testDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CMFC_testDlg 对话框



CMFC_testDlg::CMFC_testDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFC_testDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFC_testDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_SHOW, m_winShow);
	DDX_Control(pDX, IDC_STATIC_SHOW_REMOTE, m_winShowRemote);
}

BEGIN_MESSAGE_MAP(CMFC_testDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMFC_testDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CMFC_testDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CMFC_testDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CMFC_testDlg 消息处理程序

BOOL CMFC_testDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFC_testDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFC_testDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFC_testDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CFile file(TEXT("MFCfile.mpg"), CFile::modeWrite | CFile::typeBinary | CFile::modeCreate);

static void msg_cb(MyMSG* msg, void* userdata)
{
	file.Write(msg->body.sample.buf, msg->body.sample.len);
}

void CMFC_testDlg::OnBnClickedButton1()
{
	// TODO:  在此添加控件通知处理程序代码
	cc_userinfo_t info;
	strcpy(info.ip, "120.199.202.36");
	//strcpy(info.ip, "192.168.1.101");
	info.port = 5566;
#ifdef DEBUG
	info.userid = 104;
#else
	info.userid = 103;
#endif
	info.roomid = 24;
	startpreview(m_winShow.GetSafeHwnd(), 1, &info, 0, NULL);
	setmsgcallback(this, OnMsg);
	//startcapture(NULL, NULL, NULL);
	//setmsgcallback(0, msg_cb);
	//startpreview(m_winShow.GetSafeHwnd(), 1, 0, NULL);
	//startremotepreview("192.168.1.101", 0, 0, 0);
}


void CMFC_testDlg::OnBnClickedButton2()
{
	// TODO:  在此添加控件通知处理程序代码
	stoppreview(NULL, NULL);
	//stopremotepreview(1, 0, 0);
	//resizewindow();
	//stopsendsample(0, 0);
}


void CMFC_testDlg::OnBnClickedButton3()
{
	// TODO:  在此添加控件通知处理程序代码
	//setmsgcallback(this, OnMsg);
	//startremotepreview(3, m_winShowRemote.GetSafeHwnd(), 0, 0);
	//startcapture(NULL, NULL, NULL);
	startsendsample(0, 0);
}

void CMFC_testDlg::OnMsg(MyMSG* msg, void* userdata)
{
	// TODO:  在此添加控件通知处理程序代码
	CMFC_testDlg* dlg = (CMFC_testDlg*)userdata;
	switch (msg->code){
	case CODE_NEWUSER:
		dlg->do_newuser(msg);
		break;
	case CODE_USERLOGOUT:
		dlg->do_userlogout(msg);
		break;
	case CODE_SOCKETERROR:
		dlg->do_socket_error(msg);
		break;
	default:
		break;
	}
}

void CMFC_testDlg::do_newuser(MyMSG* msg)
{
	startcapture(NULL, NULL, NULL);
	startremotepreview(msg->body.userinfo.userid, m_winShowRemote.GetSafeHwnd(), 0, 0);
}

void CMFC_testDlg::do_userlogout(MyMSG* msg)
{
	AfxMessageBox(_T("received user logout!"));
	stopremotepreview(msg->body.userinfo.userid, 0, 0);
}

void CMFC_testDlg::do_socket_error(MyMSG* msg)
{
	AfxMessageBox(_T("received error!"));
	stoppreview(NULL, NULL);
	stopremotepreview(msg->body.userinfo.userid, 0, 0);
}
