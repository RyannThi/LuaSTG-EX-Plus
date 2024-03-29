﻿////////////////////////////////////////////////////////////////////////////////
/// @file  f2dWindowImpl.h
/// @brief fancy2D渲染窗口接口实现
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "fcyRefObj.h"
#include "fcyIO\fcyStream.h"

#include "../f2dWindow.h"

#include <string>
#include <unordered_map>

#include <Windows.h>

class f2dEngineImpl;
class f2dWindowImpl;

////////////////////////////////////////////////////////////////////////////////
/// @brief 窗口类
////////////////////////////////////////////////////////////////////////////////
class f2dWindowClass
{
	friend class f2dWindowImpl;
private:
	f2dEngineImpl* m_pEngine;
	std::wstring m_ClsName;
private:
	static std::unordered_map<HWND, f2dWindowImpl*> s_WindowCallBack;
	static LRESULT CALLBACK WndProc(HWND Handle, UINT Msg, WPARAM wParam, LPARAM lParam);
public:
	fcStrW GetName()const; // 获得窗口类名称
	f2dWindowImpl* CreateRenderWindow(const fcyRect& Pos, fcStrW Title, fBool Visiable, F2DWINBORDERTYPE Border, bool DisableIME = true); // 创建窗口
public:
	f2dWindowClass(f2dEngineImpl* pEngine, fcStrW ClassName);
	~f2dWindowClass();
};

////////////////////////////////////////////////////////////////////////////////
/// @brief 窗口DC
////////////////////////////////////////////////////////////////////////////////
class f2dWindowDC
{
protected:
	HBITMAP	m_hBmp;
	HDC		m_hDC;
	char*   m_pBits;

	fInt m_Width;
	fInt m_Height;
public:
	void Delete();
	void Create(int nWidth, int nHeight);
	HDC GetSafeHdc(void){
		return m_hDC;
	}
	HBITMAP GetBmpHandle(void){
		return m_hBmp;
	}
	DWORD* GetBits(void){
		return (DWORD*)m_pBits;
	}
	fInt GetWidth()
	{
		return m_Width;
	}
	fInt GetHeight()
	{
		return m_Height;
	}
public:
	f2dWindowDC();
    ~f2dWindowDC();
};

////////////////////////////////////////////////////////////////////////////////
/// @brief IME候选词列表
////////////////////////////////////////////////////////////////////////////////
class f2dIMECandidateListImpl :
	public f2dIMECandidateList
{
protected:
	fuInt m_IMETotalCandidate;          ///< @brief 候选词个数
	fuInt m_IMESelectedCandidate;       ///< @brief 选中的候选词索引
	fuInt m_IMEPageStartCandidate;      ///< @brief 当前页码中第一个候选词的索引
	fuInt m_IMEPageCandidateCount;      ///< @brief 一页的候选词个数
	std::vector<std::wstring> m_IMECandidateList; ///< @brief 候选词列表
public: // 接口实现
	fuInt GetCount();
	fuInt GetCurIndex();
	fuInt GetPageSize();
	fuInt GetPageStart();
	fcStrW GetCandidateStr(fuInt Index);
public:
	f2dIMECandidateListImpl(f2dWindowImpl* pWindow);
	f2dIMECandidateListImpl(const f2dIMECandidateListImpl& Right);
	~f2dIMECandidateListImpl();
};

////////////////////////////////////////////////////////////////////////////////
/// @brief 渲染窗口
////////////////////////////////////////////////////////////////////////////////
class f2dWindowImpl :
	public fcyRefObjImpl<f2dWindow>
{
	friend class f2dWindowClass;
	friend class f2dIMECandidateListImpl;
private:
	class DefaultListener :
		public f2dWindowEventListener
	{
	protected:
		f2dEngineImpl* m_pEngine;
		f2dWindowImpl* m_pThis;
	public:
		void OnClose();
		void OnPaint();
		void OnSize(fuInt ClientWidth, fuInt ClientHeight);
		void OnKeyDown(fuInt KeyCode, fuInt Flag);
		void OnKeyUp(fuInt KeyCode, fuInt Flag);
		void OnCharInput(fCharW CharCode, fuInt Flag);
		void OnIMEStartComposition();
		void OnIMEEndComposition();
		void OnIMEComposition(fcStrW String, fCharW CharCode);
		void OnIMEActivated(fcStrW Desc);
		void OnIMEClosed();
		void OnIMEChangeCandidate(f2dIMECandidateList* pList);
		void OnIMEOpenCandidate(f2dIMECandidateList* pList);
		void OnIMECloseCandidate(f2dIMECandidateList* pList);
		void OnMouseMove(fShort X, fShort Y, fuInt Flag);
		void OnMouseWheel(fShort X, fShort Y, fFloat Wheel, fuInt Flag);
		void OnMouseLBDown(fShort X, fShort Y, fuInt Flag);
		void OnMouseLBUp(fShort X, fShort Y, fuInt Flag);
		void OnMouseLBDouble(fShort X, fShort Y, fuInt Flag);
		void OnMouseMBDown(fShort X, fShort Y, fuInt Flag);
		void OnMouseMBUp(fShort X, fShort Y, fuInt Flag);
		void OnMouseMBDouble(fShort X, fShort Y, fuInt Flag);
		void OnMouseRBDown(fShort X, fShort Y, fuInt Flag);
		void OnMouseRBUp(fShort X, fShort Y, fuInt Flag);
		void OnMouseRBDouble(fShort X, fShort Y, fuInt Flag);
		void OnGetFocus();
		void OnLostFocus();
	public:
		DefaultListener(f2dEngineImpl* pEngine, f2dWindowImpl* pThis)
			: m_pEngine(pEngine), m_pThis(pThis) {}
	};
private:
	// 状态
	HWND m_hWnd;
	bool m_bShow;
	std::wstring m_CaptionText;

	// 监听器
	DefaultListener m_DefaultListener;
	f2dWindowEventListener* m_pListener;

	// 输入法上下文
	HIMC m_hIMC;
	bool m_bHideIME;
	std::wstring m_CurIMEDesc;          ///< @brief 输入法描述
	std::wstring m_CurIMEComposition;   ///< @brief 输入法组词文本
	fuInt m_IMETotalCandidate;          ///< @brief 候选词个数
	fuInt m_IMESelectedCandidate;       ///< @brief 选中的候选词索引
	fuInt m_IMEPageStartCandidate;      ///< @brief 当前页码中第一个候选词的索引
	fuInt m_IMEPageCandidateCount;      ///< @brief 一页的候选词个数
	std::vector<std::wstring> m_IMECandidateList; ///< @brief 候选词列表

protected: // 内部方法
	/// @brief 初始化输入法上下文
	void InitIMEContext();
	/// @brief 取消输入法上下文
	void UninitIMEContext();
	
	/// @brief 处理IME语言变化
	void HandleIMELanguageChanged();
	/// @brief 处理组词
	void HandleIMEComposition();
	/// @brief 处理选词
	void HandleIMECandidate();
public: // 接口实现
	f2dWindowEventListener* GetListener();
	fResult SetListener(f2dWindowEventListener* pListener);

	fInt GetHandle();

	F2DWINBORDERTYPE GetBorderType();
	fResult SetBorderType(F2DWINBORDERTYPE Type);
	fcStrW GetCaption();
	fResult SetCaption(fcStrW Caption);
	fBool GetVisiable();
	fResult SetVisiable(fBool Visiable);
	fcyRect GetRect();
	fResult SetRect(const fcyRect& Range);
	fcyRect GetClientRect();
	fResult SetClientRect(const fcyRect& Range);
	void MoveToCenter();
	fBool IsTopMost();
	fResult SetTopMost(fBool TopMost);
	void HideMouse(fBool bShow);

	fBool IsHideIME() { return m_bHideIME; }
	void SetHideIME(fBool v) { m_bHideIME = v; }

	fcStrW GetIMEDesc()
	{
		return m_CurIMEDesc.c_str();
	}
	fuInt GetIMEInfo(F2DIMEINFO InfoType)
	{
		switch(InfoType)
		{
		case F2DIMEINFO_CANDIDATECOUNT:
			return m_IMETotalCandidate;
		case F2DIMEINFO_CANDIDATEINDEX:
			return m_IMESelectedCandidate;
		case F2DIMEINFO_PAGESIZE:
			return m_IMEPageCandidateCount;
		case F2DIMEINFO_PAGESTART:
			return m_IMEPageStartCandidate;
		}
		return 0;
	}
	fcStrW GetIMECompString()
	{
		return m_CurIMEComposition.c_str();
	}
	fuInt GetIMECandidateCount()
	{
		return m_IMETotalCandidate;
	}
	fcStrW GetIMECandidate(fuInt Index)
	{
		if(Index > m_IMETotalCandidate)
			return NULL;
		else
			return m_IMECandidateList[Index].c_str();
	}
protected:
	f2dWindowImpl(f2dEngineImpl* pEngine, f2dWindowClass* WinCls, const fcyRect& Pos, fcStrW Title, fBool Visiable, F2DWINBORDERTYPE Border, bool DisableIME = true);
	~f2dWindowImpl();
};
