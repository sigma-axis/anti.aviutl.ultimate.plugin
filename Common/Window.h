﻿#pragma once

namespace Tools
{
	//
	// このクラスは WinAPI のウィンドウを管理します。
	//
	template<class T>
	struct WindowT
	{
		//
		// コンストラクタです。
		//
		WindowT() : hwnd(0)
		{
		}

		//
		// デストラクタです。
		//
		virtual ~WindowT()
		{
			unsubclass();
		}

		//
		// HWND を返します。
		//
		operator HWND() const { return hwnd; }

		//
		// ウィンドウの識別名を返します。
		// 安全にダウンキャストしたいときに使用してください。
		//
		virtual LPCTSTR getWindowId() { return _T(""); }

		//
		// ウィンドウを作成します。
		//
		virtual BOOL create(DWORD exStyle, LPCTSTR className, LPCTSTR windowName, DWORD style,
			int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param)
		{
			if (hwnd) return FALSE; // すでにウィンドウを作成済みの場合は失敗します。

			associator.start(this);
			HWND result = ::CreateWindowEx(exStyle, className, windowName, style, x, y, w, h, parent, menu, instance, param);
			associator.stop();

			return !!result;
		}

		//
		// ウィンドウを破壊します。
		//
		virtual BOOL destroy()
		{
			if (!hwnd) return FALSE;
			BOOL result = ::DestroyWindow(hwnd);
			hwnd = 0;
			return result;
		}

		//
		// ウィンドウをサブクラス化します。
		//
		virtual BOOL subclass(HWND hwnd)
		{
			if (this->hwnd) return FALSE;
			this->hwnd = hwnd;
			return ::SetWindowSubclass(hwnd, subclassProc, getSubclassId(), (DWORD_PTR)this);
		}

		//
		// ウィンドウのサブクラス化を解除します。
		//
		virtual BOOL unsubclass()
		{
			if (!hwnd) return FALSE;
			BOOL result = ::RemoveWindowSubclass(hwnd, subclassProc, getSubclassId());
			hwnd = 0;
			return result;
		}

		//
		// ウィンドウが破壊されたときの最終処理を行います。
		//
		virtual void postNcDestroy()
		{
			unsubclass();
		}

		//
		// 仮想関数版のウィンドウプロシージャです。
		// 派生クラスでこの関数をオーバーライドして処理してください。
		//
		virtual LRESULT onWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			switch (message)
			{
			case WM_NCDESTROY:
				{
					LRESULT lr = ::DefSubclassProc(hwnd, message, wParam, lParam);
					postNcDestroy();
					return lr;
				}
			}

			return ::DefSubclassProc(hwnd, message, wParam, lParam);
		}

		//
		// HWND に関連付けられた WindowT* を返します。
		// HWND から WindowT* を取得する必要がある場合に使用してください。
		//
		inline static WindowT* fromHandle(HWND hwnd)
		{
			DWORD_PTR refData = 0;
			::GetWindowSubclass(hwnd, subclassProc, getSubclassId(), &refData);
			return (WindowT*)refData;
		}

		//
		// HWND に関連付けられた WindowT* を返します。
		// T 型は getWindowId() が id と同じ値を返すように実装してください。
		// それにより、id で T 型かどうか確認できるようになります。
		//
		template<class T>
		static T* fromHandle(HWND hwnd, LPCTSTR id)
		{
			WindowT* window = fromHandle(hwnd);
			if (!window) return 0;
			if (_tcscmp(id, window->getWindowId()) != 0) return 0;
			return static_cast<T*>(window);
		}

		//
		// サブクラスプロシージャです。
		// 内部的に使用されます。
		//
		inline static LRESULT CALLBACK subclassProc(
			HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
			UINT_PTR id, DWORD_PTR refData)
		{
//			MY_TRACE_FUNC("0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X", hwnd, message, wParam, lParam, id, refData);

			auto window = (WindowT*)refData;
			return window->onWndProc(hwnd, message, wParam, lParam);
		}

	public:

		//
		// この構造体は HWND と WindowT* を関連付けます。
		// HWND を生成するタイミングで使用してください。
		//
		thread_local inline static struct Associator
		{
			WindowT* target; // HWND を生成中の WindowT オブジェクトです。
			HHOOK hook; // CBT フックのハンドルです。

			//
			// ウィンドウハンドルが生成される関数の直前にこの関数を呼んでください。
			//
			void start(WindowT* window)
			{
				target = window;
				if (hook) ::UnhookWindowsHookEx(hook);
				hook = ::SetWindowsHookEx(WH_CBT, hookProc, 0, ::GetCurrentThreadId());
			}

			//
			// ウィンドウハンドルが生成される関数の直後にこの関数を呼んでください。
			//
			void stop()
			{
				if (hook) ::UnhookWindowsHookEx(hook);
				hook = 0;
				target = 0;
			}

			//
			// HWND と WindowT* を双方向に関連付けます。サブクラス化も行います。
			// 内部的に使用されます。
			//
			BOOL subclass(HWND hwnd)
			{
				if (!target) return FALSE;

				return target->subclass(hwnd);
			}

			//
			// CBT フックプロシージャです。
			// 内部的に使用されます。
			//
			inline static LRESULT CALLBACK hookProc(int code, WPARAM wParam, LPARAM lParam)
			{
//				MY_TRACE_FUNC("%d, 0x%08X, 0x%08X", code, wParam, lParam);

				if (code == HCBT_CREATEWND)
				{
					associator.subclass((HWND)wParam);
					associator.stop();
				}

				return ::CallNextHookEx(0, code, wParam, lParam);
			}
		} associator = {};

		//
		// サブクラスID用の変数です。
		// 内部的に使用されます。
		//
		inline static UINT SubclassIdPlacement = 0;

		//
		// サブクラスIDを返します。
		// 内部的に使用されます。
		//
		inline static UINT_PTR getSubclassId() { return (UINT_PTR)&SubclassIdPlacement; }

		//
		// ウィンドウハンドルです。
		//
		HWND hwnd;
	};

	//
	// このクラスはWindowTの実体です。
	// WindowT<Window>::SubclassIdPlacementへのポインタがサブクラスIDになります。
	// 重複してサブクラス化する場合は struct Hoge : WindowT<Hoge> のように別のクラスとして実体化してください。
	//
	struct Window : WindowT<Window>
	{
	};
}
