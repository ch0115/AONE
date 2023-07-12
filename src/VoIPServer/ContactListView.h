﻿#pragma once



// CContactListView 폼 보기

class CContactListView : public CFormView
{
	DECLARE_DYNCREATE(CContactListView)

protected:
	CContactListView();           // 동적 만들기에 사용되는 protected 생성자입니다.
	virtual ~CContactListView();

public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LIST_VIEW };
#endif
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif
	
	CMFCButton m_btnEnable;
	CMFCButton m_btnDisable;
	CMFCButton m_btnDelete;
	CMFCButton m_btnContactRefresh;
	CMFCButton m_btnConfRefresh;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual void OnInitialUpdate();
	CListCtrl m_ltContactNames;
	CListCtrl m_ltConferenceNames;
	INT m_ltIndex;
	afx_msg void OnNMClickLtContact(NMHDR* pNMHDR, LRESULT* pResult);
	LRESULT processUiControlNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedMfcbtnChangeState();
	afx_msg void OnBnClickedMfcbtnDelete();
	afx_msg void OnBnClickedMfcbtnChangeDisable();
	void UpdateContactView();
	void UpdateConferenceView();
	afx_msg void OnBnClickedMfcbtnRefreshContact();
	afx_msg void OnBnClickedMfcbtnRefreshConf();

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};


