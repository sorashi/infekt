/**
 * Copyright (C) 2010 cxxjoe
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 **/

#include "stdafx.h"
#include "nfo_view_ctrl.h"

#define NFOVWR_CTRL_CLASS_NAME _T("RenderedNfoViewCtrl")


CNFOViewControl::CNFOViewControl(HINSTANCE a_hInstance, HWND a_parent) : CNFORenderer()
{
	m_instance = a_hInstance;
	m_parent = a_parent;
	m_left = m_top = m_width = m_height = 0;
	m_hwnd = 0;
	m_cursor = IDC_ARROW;

	m_selStartRow = m_selStartCol = m_selEndRow = m_selEndCol = (size_t)-1;
	m_leftMouseDown = m_movedDownMouse = false;
}


bool CNFOViewControl::CreateControl(int a_left, int a_top, int a_width, int a_height)
{
	WNDCLASSEX l_class = {0};
	l_class.cbSize = sizeof(WNDCLASSEX);

	l_class.hInstance = m_instance;
	l_class.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	l_class.lpszClassName = NFOVWR_CTRL_CLASS_NAME;
	l_class.lpfnWndProc = &_WindowProc;
	l_class.hCursor = ::LoadCursor(NULL, IDC_ARROW);

	if(::RegisterClassEx(&l_class) == 0)
	{
		return false;
	}

	m_top = a_top;
	m_left = a_left;
	m_width = a_width;
	m_height = a_height;

	m_hwnd = ::CreateWindowEx(WS_EX_CLIENTEDGE,
		NFOVWR_CTRL_CLASS_NAME, NULL,
		WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE | WS_BORDER,
		m_left, m_top, m_width, m_height,
		m_parent, NULL,
		m_instance, NULL);

	// :TODO: allow window to receive focus.
	// :TODO: allow text selection...

	if(!m_hwnd)
	{
		return false;
	}

	::SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (INT_PTR)this);

	UpdateScrollbars();

	return true;
}


void CNFOViewControl::UpdateScrollbars()
{
	bool l_needsV, l_needsH;
	int l_vWidth, l_hHeight;

	// init struct:
	SCROLLINFO l_si = {0};
	l_si.cbSize = sizeof(SCROLLINFO);

	// get scroll bar dimensions:
	l_vWidth = ::GetSystemMetrics(SM_CXVSCROLL);
	l_hHeight = ::GetSystemMetrics(SM_CXHSCROLL);

	// determine which scrollbars we need:
	l_needsH = (m_width - l_vWidth < (int)GetWidth());
	l_needsV = (m_height - l_hHeight < (int)GetHeight());
	// ... we do this to add some extra space between
	// content and scrollbars for nicer looks.

	// set common flags:
	l_si.fMask = SIF_RANGE | SIF_PAGE;
	l_si.nMin = 0;

	// update horizontal scrollbar info:
	l_si.nPage = m_width / m_blockWidth;
	l_si.nMax = GetWidth() / m_blockWidth + (l_needsV ? 2 : 0);
	::SetScrollInfo(m_hwnd, SB_HORZ, &l_si, FALSE);

	// update vertical scrollbar info:
	l_si.nPage = m_height / m_blockHeight;
	l_si.nMax = GetHeight() / m_blockHeight + (l_needsH ? 1 : 0);
	::SetScrollInfo(m_hwnd, SB_VERT, &l_si, FALSE);
}


LRESULT CNFOViewControl::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_PAINT:
		OnPaint();
		return 0;
	case WM_ERASEBKGND:
		return 0; // we erase the background in OnPaint, if necessary.
	case WM_VSCROLL:
		HandleScrollEvent(SB_VERT, LOWORD(wParam), 0);
		return 0;
	case WM_HSCROLL:
		HandleScrollEvent(SB_HORZ, LOWORD(wParam), 0);
		return 0;
	case WM_MOUSEWHEEL:
		{
			UINT l_lines = 0;
			if(::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &l_lines, 0))
			{
				int l_delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				HandleScrollEvent(SB_VERT, INT_MIN, -l_delta * l_lines);
			}
		}
		return 0;
	case WM_MOUSEHWHEEL: // Windows Vista & higher only...
		{
			UINT l_chars = 0;
			if(::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &l_chars, 0))
			{
				int l_delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				HandleScrollEvent(SB_HORZ, INT_MIN, -l_delta * l_chars);
			}
		}
		// Source: http://msdn.microsoft.com/en-us/library/ms997498.aspx#mshrdwre_topic2
		// The MSDN page for WM_MOUSEHWHEEL says "return zero" though... wait till someone complains.
		return TRUE;
	case WM_SIZE:
		m_width = LOWORD(lParam);
		m_height = HIWORD(lParam);
		UpdateScrollbars();
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_SETCURSOR:
		OnSetCursor();
		return TRUE;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		OnMouseClickEvent(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MBUTTONUP:
		CopySelectedTextToClipboard();
		return 0;

	default:
		return ::DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
}


void CNFOViewControl::OnPaint()
{
	// paint/dc helper business:
	HDC l_dc;
	PAINTSTRUCT l_ps;
	cairo_surface_t *l_surface, *l_realSurface = NULL;
	bool l_buffer = (m_selStartRow != (size_t)-1 && m_selEndRow != (size_t)-1);

	// get scroll offsets:
	int l_x, l_y;
	GetScrollPositions(l_x, l_y);

	// let's paint!
	l_dc = ::BeginPaint(m_hwnd, &l_ps);
	l_buffer = l_buffer || l_ps.fErase; // avoid flickering... meh :TODO: figure out better way to erase the background
	l_surface = cairo_win32_surface_create(l_dc);

	if(l_buffer)
	{
		l_realSurface = l_surface;
		l_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_width, m_height);
	}

	// erase the background if necessary:
	if(l_ps.fErase)
	{
		cairo_t* l_cr = cairo_create(l_surface);
		cairo_set_source_rgb(l_cr, S_COLOR_T_CAIRO(m_backColor));
		cairo_paint(l_cr);
		cairo_destroy(l_cr);
	}

	// draw draw draw fight the powa!
	DrawToSurface(l_surface, 0, 0, l_x * m_blockWidth, l_y * m_blockHeight, m_width, m_height);

	// draw highlighted (selected) text:
	if(m_selStartRow != (size_t)-1 && m_selEndRow != (size_t)-1)
	{
		S_COLOR_T l_back = m_backColor.Invert();
		RenderText(m_textColor.Invert(), &l_back, m_hyperLinkColor.Invert(),
			m_selStartRow, m_selStartCol, m_selEndRow, m_selEndCol,
			l_surface, -l_x * m_blockWidth, -l_y * m_blockHeight);
	}

	// clean up:
	if(l_realSurface)
	{
		cairo_t *cr = cairo_create(l_realSurface);
		cairo_set_source_surface(cr, l_surface, 0, 0);
		cairo_paint(cr);
		cairo_surface_destroy(l_realSurface);
	}
	cairo_surface_destroy(l_surface);
	::EndPaint(m_hwnd, &l_ps);
}


void CNFOViewControl::OnMouseMove(int a_x, int a_y)
{
	size_t l_row, l_col;
	CalcFromMouseCoords(a_x, a_y, l_row, l_col);

	if(!m_leftMouseDown && m_nfo->GetLink(l_row, l_col) != NULL)
	{
		m_cursor = IDC_HAND;
	}
	else if(IsTextChar(l_row, l_col))
	{
		m_cursor = IDC_IBEAM;
	}
	else
	{
		m_cursor = IDC_ARROW;
	}

	if(m_leftMouseDown && !m_movedDownMouse)
	{
		if(l_row != m_selStartRow || l_col != m_selStartCol)
		{
			m_movedDownMouse = true;
		}
	}
	
	if(m_leftMouseDown && m_movedDownMouse)
	{
		if(m_selEndRow != l_row || m_selEndCol != l_col)
		{
			m_selEndRow = l_row;
			m_selEndCol = l_col;
			RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);
		}
	}
}


void CNFOViewControl::OnSetCursor()
{
	::SetCursor(::LoadCursor(NULL, m_cursor));
}


void CNFOViewControl::OnMouseClickEvent(UINT a_event, int a_x, int a_y)
{
	size_t l_row, l_col;
	CalcFromMouseCoords(a_x, a_y, l_row, l_col);


	if(a_event == WM_LBUTTONDOWN)
	{
		if(m_selStartRow != (size_t)-1)
		{
			// clear previous selection
			m_selStartRow = m_selEndRow = (size_t)-1;
			m_selStartCol = m_selEndCol = (size_t)-1;
			RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);
		}

		m_leftMouseDown = true;
		m_movedDownMouse = false;
		m_selStartRow = l_row;
		m_selStartCol = l_col;
	}
	else if(a_event == WM_LBUTTONUP)
	{
		if(m_leftMouseDown)
		{
			if(m_movedDownMouse)
			{
				m_selEndRow = l_row;
				m_selEndCol = l_col;
			}
			else
			{
				m_selStartRow = m_selEndRow = (size_t)-1;
				m_selStartCol = m_selEndCol = (size_t)-1;
			}
			m_leftMouseDown = false;
			RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);
		}
		
		if(!m_movedDownMouse)
		{
			const CNFOHyperLink *l_link = m_nfo->GetLink(l_row, l_col);

			if(l_link)
			{
				::ShellExecute(NULL, _T("open"), l_link->GetHref().c_str(),
					NULL, NULL, SW_SHOW);
			}
		}
	}
	else if(a_event == WM_LBUTTONDBLCLK)
	{
		if(IsTextChar(l_row, l_col))
		{
			size_t lc = l_col;

			m_selStartRow = m_selEndRow = l_row;
			m_selStartCol = m_selEndCol = l_col;

			while(IsTextChar(l_row, lc - 1) && iswalnum(m_nfo->GetGridChar(l_row, lc - 1)))
			{
				m_selStartCol = --lc;
			}

			lc = l_col;
			while(IsTextChar(l_row, lc + 1) && iswalnum(m_nfo->GetGridChar(l_row, lc + 1)))
			{
				m_selEndCol = ++lc;
			}

			RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);
		}
	}
}


void CNFOViewControl::CalcFromMouseCoords(int a_x, int a_y, size_t& ar_row, size_t& ar_col)
{
	int l_x, l_y;
	GetScrollPositions(l_x, l_y);

	// calc real positions:
	ar_row = l_y + (a_y / m_blockHeight) - 1;
	ar_col = l_x + (a_x / m_blockWidth) - 1;
	// :TODO: find out why we need to subtract 1.
}


void CNFOViewControl::GetScrollPositions(int& ar_x, int& ar_y)
{
	SCROLLINFO l_si = {0};

	l_si.cbSize = sizeof(SCROLLINFO);
	l_si.fMask = SIF_POS;

	::GetScrollInfo(m_hwnd, SB_HORZ, &l_si);
	ar_x = l_si.nPos;

	::GetScrollInfo (m_hwnd, SB_VERT, &l_si);
	ar_y = l_si.nPos;
}


void CNFOViewControl::HandleScrollEvent(int a_dir, int a_event, int a_change)
{
	int l_prevPos;

	SCROLLINFO l_si = {0};
	l_si.cbSize = sizeof(SCROLLINFO);
	l_si.fMask = SIF_ALL;

	// Save the position for comparison later on:
	::GetScrollInfo(m_hwnd, a_dir, &l_si);
	l_prevPos = l_si.nPos;

#if (SB_LINEUP != SB_LINELEFT) || (SB_LINEDOWN != SB_LINERIGHT) || (SB_PAGEDOWN != SB_PAGERIGHT) || (SB_PAGEUP != SB_PAGELEFT)
#error ZOMG!
#endif

	switch(a_event)
	{
	case INT_MIN:
		l_si.nPos += a_change;
		break;
	case SB_TOP: // user hit the HOME keyboard key
		l_si.nPos = l_si.nMin;
		break;
	case SB_BOTTOM: // user hit the END keyboard key
		l_si.nPos = l_si.nMax;
		break;
	case SB_LINEUP: // user clicked the top/left arrow
		l_si.nPos -= 1;
		break;
	case SB_LINEDOWN: // user clicked the bottom/right arrow
		l_si.nPos += 1;
		break;
	case SB_PAGEUP: // user clicked the scroll bar shaft above/left of the scroll box
		l_si.nPos -= l_si.nPage;
		break;
	case SB_PAGEDOWN: // user clicked the scroll bar shaft below/right of the scroll box
		l_si.nPos += l_si.nPage;
		break;
	case SB_THUMBTRACK: // user dragged the scroll box
		l_si.nPos = l_si.nTrackPos;
		break;
	}

	// Set the position and then retrieve it.  Due to adjustments
	//   by Windows it may not be the same as the value set.
	l_si.fMask = SIF_POS;
	::SetScrollInfo(m_hwnd, a_dir, &l_si, TRUE);
	::GetScrollInfo(m_hwnd, a_dir, &l_si);

	// If the position has changed, scroll the window:
	if(l_si.nPos != l_prevPos)
	{
		::ScrollWindow(m_hwnd, (a_dir == SB_HORZ ? m_blockWidth * (l_prevPos - l_si.nPos) : 0),
			(a_dir == SB_VERT ? m_blockHeight * (l_prevPos - l_si.nPos) : 0),
			NULL, NULL);
	}
}


LRESULT CALLBACK CNFOViewControl::_WindowProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CNFOViewControl *l_ctrl = (CNFOViewControl*)(void*)(INT_PTR)::GetWindowLongPtr(hWindow, GWLP_USERDATA);

	if(l_ctrl)
	{
		return l_ctrl->WindowProc(uMsg, wParam, lParam);
	}

	return ::DefWindowProc(hWindow, uMsg, wParam, lParam);
}


const std::wstring CNFOViewControl::GetSelectedText() const
{
	if(m_selStartRow == (size_t)-1 || m_selEndRow == (size_t)-1)
		return L"";

	std::wstring l_text;
	size_t l_leftStart = std::numeric_limits<size_t>::max();
	int l_dryRun = 1;

	do 
	{
		for(size_t row = m_selStartRow; row <= m_selEndRow; row++)
		{
			bool l_textStarted = false;

			for(size_t col = 0; col < m_gridData->GetCols(); col++)
			{
				if(m_selStartRow != (size_t)-1)
				{
					if(row == m_selStartRow && col < m_selStartCol)
						continue;
					else if(row == m_selEndRow && col > m_selEndCol)
						break;
				}

				if(!IsTextChar(row, col, true))
				{
					if(!l_dryRun && l_textStarted) l_text += L' ';
					continue;
				}

				if(l_dryRun)
				{
					if(col < l_leftStart) l_leftStart = col;
				}
				else
				{
					if(!l_textStarted)
					{
						for(size_t p = 0; p < col - l_leftStart; p++)
							l_text += L' ';
					}

					l_text += m_nfo->GetGridChar(row, col);
				}

				l_textStarted = true;
			}

			if(!l_dryRun)
			{
				while(l_text.size() && l_text[l_text.size() - 1] == L' ') l_text.erase(l_text.size() - 1);

				if(row != m_selEndRow)
					l_text += L"\r\n"; // SetClipboardContent with CF_UNICODETEXT wants \r\n instead of \n...
			}
		}
	} while(l_dryRun--);

	return l_text;
}


void CNFOViewControl::CopySelectedTextToClipboard() const
{
	const std::wstring l_wstr = GetSelectedText();

	if(::OpenClipboard(m_hwnd))
	{
		HGLOBAL l_hGlobal = ::GlobalAlloc(GMEM_MOVEABLE,
			sizeof(wchar_t) * (l_wstr.size() + 1));

		if(l_hGlobal)
		{
			wchar_t* l_hCopy = (wchar_t*)GlobalLock(l_hGlobal);

			memcpy(l_hCopy, l_wstr.c_str(), sizeof(wchar_t) * l_wstr.size());
			l_hCopy[l_wstr.size()] = 0;
			GlobalUnlock(l_hCopy); 

			EmptyClipboard();

			::SetClipboardData(CF_UNICODETEXT, l_hGlobal);
		}

		::CloseClipboard();
	}
}


CNFOViewControl::~CNFOViewControl()
{
	::UnregisterClass(NFOVWR_CTRL_CLASS_NAME, m_instance);

	if(m_hwnd)
	{
		::DestroyWindow(m_hwnd);
		// MSDN: "If the specified window is a parent window, DestroyWindow
		// automatically destroys the associated child windows."
	}
}
