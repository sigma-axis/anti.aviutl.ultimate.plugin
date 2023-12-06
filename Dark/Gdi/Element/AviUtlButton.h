﻿#pragma once
#include "Skin.h"
#include "Gdi/Renderer.h"

namespace fgo::dark::gdi
{
	inline struct AviUtlButtonRenderer : Renderer
	{
		int onFillRect(State* currentState, HDC dc, LPCRECT rc, HBRUSH brush) override
		{
			MY_TRACE_FUNC("0x%08X, (%ws), 0x%08X", dc, hive.safe_string(rc), brush);

			return hive.orig.FillRect(dc, rc, brush);
		}

		BOOL onDrawFrame(State* currentState, HDC dc, LPRECT rc, UINT width, UINT type) override
		{
			MY_TRACE_FUNC("0x%08X, (%ws), %d, 0x%08X", dc, hive.safe_string(rc), width, type);

			return hive.orig.DrawFrame(dc, rc, width, type);
		}

		BOOL onDrawFrameControl(State* currentState, HDC dc, LPRECT rc, UINT type, UINT state) override
		{
			MY_TRACE_FUNC("0x%08X, (%ws), 0x%08X, 0x%08X", dc, hive.safe_string(rc), type, state);

			return hive.orig.DrawFrameControl(dc, rc, type, state);
		}

		BOOL onFrameRect(State* currentState, HDC dc, LPCRECT rc, HBRUSH brush) override
		{
			MY_TRACE_FUNC("0x%08X, (%ws), 0x%08X", dc, hive.safe_string(rc), brush);

			return hive.orig.FrameRect(dc, rc, brush);
		}

		BOOL onDrawEdge(State* currentState, HDC dc, LPRECT rc, UINT edge, UINT flags) override
		{
			MY_TRACE_FUNC("0x%08X, (%ws), 0x%08X, 0x%08X", dc, hive.safe_string(rc), edge, flags);

			HWND hwnd = currentState->hwnd;
			HICON icon = (HICON)::GetWindowLongPtr(hwnd, 0);
			int x = rc->left;
			int y = rc->top;
			int w = rc->right - rc->left;
			int h = rc->bottom - rc->top;

			HTHEME theme = skin::theme::manager.getTheme(skin::theme::THEME_BUTTON);
			int partId = BP_PUSHBUTTON;
			int stateId = PBS_NORMAL;

			if (edge == EDGE_SUNKEN)
			{
				x += 1;
				y += 1;
				stateId = PBS_PRESSED;
			}

			RECT rc2 = *rc;
			skin::theme::manager.onDrawThemeBackground(theme, dc, partId, stateId, &rc2);

			::DrawIconEx(dc, x, y, icon, w, h, 0, NULL, DI_NORMAL);

			return TRUE;
		}

		BOOL onDrawFocusRect(State* currentState, HDC dc, LPCRECT rc) override
		{
			MY_TRACE_FUNC("0x%08X, 0x%08X, (%ws)", dc, hive.safe_string(rc));

			return hive.orig.DrawFocusRect( dc, rc);
		}

		BOOL onDrawStateW(State* currentState, HDC dc, HBRUSH fore, DRAWSTATEPROC cb, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT flags) override
		{
			MY_TRACE_FUNC("0x%08X, 0x%08X, %d, %d, %d, %d, 0x%08X", dc, fore, x, y, cx, cy, flags);

			return hive.orig.DrawStateW(dc, fore, cb, lData, wData, x, y, cx, cy, flags);
		}

		BOOL onExtTextOutW(State* currentState, HDC dc, int x, int y, UINT options, LPCRECT rc, LPCWSTR text, UINT c, CONST INT* dx) override
		{
			MY_TRACE_FUNC("%d, %d, 0x%08X, 0x%08X, 0x%08X, %d, 0x%08X, 0x%08X, 0x%08X", x, y, options, rc, text, c, dx, ::GetBkColor(dc), ::GetTextColor(dc));

			return hive.orig.ExtTextOutW(dc, x, y, options, rc, text, c, dx);
		}

		BOOL onPatBlt(State* currentState, HDC dc, int x, int y, int w, int h, DWORD rop) override
		{
			MY_TRACE_FUNC("0x%08X, %d, %d, %d, %d, 0x%08X", dc, x, y, w, h, rop);

			return hive.orig.PatBlt(dc, x, y, w, h, rop);
		}
	} aviutlButtonRenderer;
}