﻿#pragma once
#include "Resource.h"
#include "SubWindow.h"
#include "SubProcess/PSDToolKit.h"
#include "SubProcess/Bouyomisan.h"

namespace fgo::nest
{
	//
	// このクラスはメインウィンドウです。
	// このウィンドウはシャトルをドッキングさせることができます。
	//
	struct MainWindow : DockSite<Tools::Window>
	{
		struct CommandID
		{
			static const UINT SUB_WINDOW_BEGIN = 100;
			static const UINT SUB_WINDOW_END = 200;
			static const UINT SUB_PROCESS_BEGIN = 200;
			static const UINT SUB_PROCESS_END = 300;
			static const UINT SHOW_CONFIG_DIALOG = 1000;
			static const UINT IMPORT_LAYOUT = 1001;
			static const UINT EXPORT_LAYOUT = 1002;
			static const UINT CREATE_SUB_WINDOW = 1003;

			static const UINT MAXIMIZE_PLAY = 30000;
		};

		struct Loader {
			virtual HRESULT loadConfig(LPCWSTR fileName, BOOL _import) = 0;
		}; std::shared_ptr<Loader> loader;

		struct Saver {
			virtual HRESULT saveConfig(LPCWSTR fileName, BOOL _export) = 0;
		}; std::shared_ptr<Saver> saver;

		HMENU subWindowMenu = 0;
		HMENU subProcessMenu = 0;

		//
		// コンストラクタです。
		//
		MainWindow()
		{
		}

		//
		// デストラクタです。
		//
		virtual ~MainWindow()
		{
			MY_TRACE(_T("MainWindow::~MainWindow(), 0x%08X\n"), (HWND)*this);
		}

		//
		// メインウィンドウを作成します。
		//
		BOOL create(const std::shared_ptr<Loader>& loader, const std::shared_ptr<Saver>& saver)
		{
			MY_TRACE(_T("MainWindow::create()\n"));

			this->loader = loader;
			this->saver = saver;

			//「AoiSupport」用の処理です。
			// クラス名を"AviUtl"にして、AviUtlウィンドウに偽装します。
			static const LPCTSTR ClassName = _T("AviUtl");

			WNDCLASS wc = {};
			wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
			wc.hCursor = ::LoadCursor(0, IDC_ARROW);
			wc.lpfnWndProc = ::DefWindowProc;
			wc.hInstance = hive.instance;
			wc.lpszClassName = ClassName;
			::RegisterClass(&wc);

			return Tools::Window::create(
				0,
				_T("Nest"), // フックしてあとで"AviUtl"に置き換えます。
				_T("Nest"),
				WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME |
				WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				0, 0, hive.instance, 0);
		}

		//
		// システムメニューを初期化します。
		//
		void initSystemMenu()
		{
			HMENU menu = ::GetSystemMenu(*this, FALSE);
			int index = 0;
			::InsertMenu(menu, index++, MF_BYPOSITION | MF_POPUP, (UINT)subWindowMenu, _T("サブウィンドウ"));
			::InsertMenu(menu, index++, MF_BYPOSITION | MF_POPUP, (UINT)subProcessMenu, _T("サブプロセス"));
			::InsertMenu(menu, index++, MF_BYPOSITION | MF_STRING, CommandID::CREATE_SUB_WINDOW, _T("サブウィンドウを新規作成"));
			::InsertMenu(menu, index++, MF_BYPOSITION | MF_STRING, CommandID::IMPORT_LAYOUT, _T("レイアウトのインポート"));
			::InsertMenu(menu, index++, MF_BYPOSITION | MF_STRING, CommandID::EXPORT_LAYOUT, _T("レイアウトのエクスポート"));
			::InsertMenu(menu, index++, MF_BYPOSITION | MF_STRING, CommandID::SHOW_CONFIG_DIALOG, _T("ネストの設定"));
			::InsertMenu(menu, index++, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
		}

		//
		// サブウィンドウ一覧用メニューを更新します。
		//
		void updateSubWindowMenu()
		{
			// すべての項目を削除します。
			while (::DeleteMenu(subWindowMenu, 0, MF_BYPOSITION)){}

			int c = (int)subWindowManager.collection.size();
			for (int i = 0; i < c; i++)
			{
				HWND subWindow = subWindowManager.collection[i];

				MENUITEMINFO mii = { sizeof(mii) };
				mii.fMask = MIIM_STRING | MIIM_DATA | MIIM_ID | MIIM_STATE;

				TCHAR windowName[MAX_PATH] = {};
				::GetWindowText(subWindow, windowName, MAX_PATH);

				if (::IsWindowVisible(subWindow))
					mii.fState = MFS_CHECKED;
				else
					mii.fState = MFS_UNCHECKED;

				mii.wID = CommandID::SUB_WINDOW_BEGIN + i;
				mii.dwItemData = (DWORD)subWindow;
				mii.dwTypeData = windowName;

				::InsertMenuItem(subWindowMenu, i, TRUE, &mii);
			}
		}

		//
		// サブプロセス一覧用メニューを更新します。
		//
		void updateSubProcessMenu()
		{
			// すべての項目を削除します。
			while (::DeleteMenu(subProcessMenu, 0, MF_BYPOSITION)){}

			int c = (int)subProcessManager.collection.size();
			for (int i = 0; i < c; i++)
			{
				HWND subProcess = subProcessManager.collection[i];

				MENUITEMINFO mii = { sizeof(mii) };
				mii.fMask = MIIM_STRING | MIIM_DATA | MIIM_ID | MIIM_STATE;

				TCHAR windowName[MAX_PATH] = {};
				::GetWindowText(subProcess, windowName, MAX_PATH);

				if (::IsWindowVisible(subProcess))
					mii.fState = MFS_CHECKED;
				else
					mii.fState = MFS_UNCHECKED;

				mii.wID = CommandID::SUB_PROCESS_BEGIN + i;
				mii.dwItemData = (DWORD)subProcess;
				mii.dwTypeData = windowName;

				::InsertMenuItem(subProcessMenu, i, TRUE, &mii);
			}
		}

		//
		// メインメニューを更新します。
		//
		void updateMainMenu()
		{
			HMENU menu = ::GetMenu(*this);

			LPCTSTR text = _T("再生時最大化 OFF");
			if (hive.showPlayer) text = _T("再生時最大化 ON");

			MENUITEMINFO mii = { sizeof(mii) };
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = (LPTSTR)text;
			::SetMenuItemInfo(menu, CommandID::MAXIMIZE_PLAY, FALSE, &mii);

			::DrawMenuBar(*this);
		}

		//
		// すべてのドックサイトのレイアウトを再計算します。
		//
		void calcAllLayout()
		{
			calcLayout(*this);

			for (auto subWindow : subWindowManager.collection)
				calcLayout(subWindow);
		}

		//
		// ファイルから設定を読み込みます。
		// 設定ファイルのファイル名は内部的に決定されます。
		//
		BOOL loadConfig()
		{
			MY_TRACE(_T("MainWindow::loadConfig()\n"));

			return loadConfig(magi.getConfigFileName(L"Nest.xml").c_str(), FALSE);
		}

		//
		// ファイルから設定を読み込みます。
		// _importがTRUEの場合は、現在の設定を残しつつレイアウトだけをファイルから読み込みます。
		//
		BOOL loadConfig(LPCWSTR fileName, BOOL _import)
		{
			MY_TRACE(_T("MainWindow::loadConfig(%ws, %d)\n"), fileName, _import);

			HRESULT hr = loader->loadConfig(fileName, _import);

			// メインメニューを更新します。
			updateMainMenu();

			// レイアウトを再計算します。
			calcAllLayout();

			// メインウィンドウが非表示なら表示します。
			if (!::IsWindowVisible(hwnd))
				::ShowWindow(hwnd, SW_SHOW);

			// メインウィンドウをフォアグラウンドにします。
			::SetForegroundWindow(hwnd);
			::SetActiveWindow(hwnd);

			return hr == S_OK;
		}

		//
		// ユーザーが指定したファイルからレイアウトをインポートします。
		//
		BOOL importLayout(HWND hwnd)
		{
			// ファイル選択ダイアログを表示してファイル名を取得します。

			// Shiftキーが押されている場合はレイアウトだけのインポートではなく、
			// すべての設定を完全に読み込みます。
			BOOL import = ::GetKeyState(VK_SHIFT) >= 0;

			// ユーザーが指定したファイル名が格納されるバッファです。
			WCHAR fileName[MAX_PATH] = {};

			// ファイルダイアログを表示します。
			OPENFILENAMEW ofn = { sizeof(ofn) };
			ofn.hwndOwner = hwnd;
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.lpstrTitle = L"レイアウトのインポート";
			ofn.lpstrFile = fileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFilter = L"レイアウトファイル (*.xml)\0*.xml\0" "すべてのファイル (*.*)\0*.*\0";
			ofn.lpstrDefExt = L"xml";
			if (!::GetOpenFileNameW(&ofn))
				return FALSE;

			// レイアウトファイルをインポートします。
			return loadConfig(fileName, import);
		}

		//
		// ファイルに設定を保存します。
		// 設定ファイルのファイル名は内部的に決定されます。
		//
		BOOL saveConfig()
		{
			MY_TRACE(_T("MainWindow::saveConfig()\n"));

			return saveConfig(magi.getConfigFileName(L"Nest.xml").c_str(), FALSE);
		}

		//
		// ファイルに設定を保存します。
		// _exportがTRUEの場合は、レイアウトだけをファイルに保存します。
		//
		BOOL saveConfig(LPCWSTR fileName, BOOL _export)
		{
			MY_TRACE(_T("MainWindow::saveConfig(%ws, %d)\n"), fileName, _export);

			return saver->saveConfig(fileName, _export) == S_OK;
		}

		//
		// ユーザーが指定したファイルにレイアウトをエクスポートします。
		//
		BOOL exportLayout(HWND hwnd)
		{
			// ファイル選択ダイアログを表示してファイル名を取得します。

			// ユーザーが指定したファイル名が格納されるバッファです。
			WCHAR fileName[MAX_PATH] = {};

			// ファイルダイアログを表示します。
			OPENFILENAMEW ofn = { sizeof(ofn) };
			ofn.hwndOwner = hwnd;
			ofn.Flags = OFN_OVERWRITEPROMPT;
			ofn.lpstrTitle = L"レイアウトのエクスポート";
			ofn.lpstrFile = fileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFilter = L"レイアウトファイル (*.xml)\0*.xml\0" "すべてのファイル (*.*)\0*.*\0";
			ofn.lpstrDefExt = L"xml";
			if (!::GetSaveFileNameW(&ofn))
				return FALSE;

			// レイアウトファイルをエクスポートします。
			return saveConfig(fileName, TRUE);
		}

		//
		// コンフィグダイアログを表示します。
		//
		int showConfigDialog(HWND parent)
		{
			ConfigDialog dialog;

			return dialog.doModal(parent);
		}

		//
		// サブウィンドウを作成します。
		//
		BOOL createSubWindow(HWND parent)
		{
			TCHAR name[MAX_PATH] = {};
			SubWindow::getEmptyName(name, std::size(name));
			MY_TRACE_TSTR(name);

			auto subWindow = std::make_shared<SubWindow>();
			if (subWindow->create(name, parent))
				subWindow->init(name, *subWindow);

			return TRUE;
		}

		//
		// サブプロセス用のウィンドウを作成します。
		//
		BOOL createSubProcesses(HWND parent)
		{
			if (hive.dockPSDToolKit) {
				const LPCTSTR name = Hive::PSDToolKitName;
				auto shuttle = psdtoolkit = std::make_shared<PSDToolKit>();
				shuttle->create(name, *this);
				shuttle->init(name, *shuttle);
			}

			if (hive.dockBouyomisan) {
				const LPCTSTR name = Hive::BouyomisanName;
				auto shuttle = bouyomisan = std::make_shared<Bouyomisan>();
				shuttle->create(name, *this);
				shuttle->init(name, *shuttle);
			}

			return TRUE;
		}

		//
		// ウィンドウプロシージャです。
		//
		LRESULT onWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override
		{
			switch (message)
			{
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_CHAR:
			case WM_DEADCHAR:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_SYSCHAR:
			case WM_SYSDEADCHAR:
				{
					MY_TRACE(_T("MainWindow::onWndProc(WM_***KEY***, 0x%08X, 0x%08X, 0x%08X)\n"), message, wParam, lParam);

					return ::SendMessage(hive.aviutlWindow, message, wParam, lParam);
				}
			case WM_ACTIVATE: // 「patch.aul」用の処理です。
				{
					MY_TRACE(_T("MainWindow::onWndProc(WM_ACTIVATE, 0x%08X, 0x%08X)\n"), wParam, lParam);

					if (LOWORD(wParam) == WA_INACTIVE)
						::SendMessage(hive.aviutlWindow, message, wParam, lParam);

					break;
				}
			case WM_MENUSELECT: // 「patch.aul」用の処理です。
				{
					MY_TRACE(_T("MainWindow::onWndProc(WM_MENUSELECT, 0x%08X, 0x%08X)\n"), wParam, lParam);

					::SendMessage(hive.aviutlWindow, message, wParam, lParam);

					break;
				}
			case WM_CLOSE:
				{
					MY_TRACE(_T("MainWindow::onWndProc(WM_CLOSE, 0x%08X, 0x%08X)\n"), wParam, lParam);

					return ::SendMessage(hive.aviutlWindow, message, wParam, lParam);
				}
			case WM_COMMAND:
				{
					MY_TRACE(_T("MainWindow::onWndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

					UINT id = LOWORD(wParam);

					switch (id)
					{
					case CommandID::MAXIMIZE_PLAY:
						{
							MY_TRACE(_T("CommandID::MAXIMIZE_PLAY\n"));

							hive.showPlayer = !hive.showPlayer;

							updateMainMenu();

							break;
						}
					}

					return ::SendMessage(hive.aviutlWindow, message, wParam, lParam);
				}
			case WM_SYSCOMMAND:
				{
					MY_TRACE(_T("MainWindow::onWndProc(WM_SYSCOMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

					if (wParam >= CommandID::SUB_WINDOW_BEGIN &&
						wParam < CommandID::SUB_WINDOW_END)
					{
						// メニューで選択されたサブウィンドウの表示/非表示を切り替えます。

						MENUITEMINFO mii = { sizeof(mii) };
						mii.fMask = MIIM_DATA;
						::GetMenuItemInfo(subWindowMenu, wParam, FALSE, &mii);

						HWND subWindow = (HWND)mii.dwItemData;
						if (subWindow)
							::SendMessage(subWindow, WM_CLOSE, 0, 0);

						break;
					}

					if (wParam >= CommandID::SUB_PROCESS_BEGIN &&
						wParam < CommandID::SUB_PROCESS_END)
					{
						// メニューで選択されたサブプロセスの表示/非表示を切り替えます。

						MENUITEMINFO mii = { sizeof(mii) };
						mii.fMask = MIIM_DATA;
						::GetMenuItemInfo(subProcessMenu, wParam, FALSE, &mii);

						HWND subProcess = (HWND)mii.dwItemData;
						if (subProcess)
							::SendMessage(subProcess, WM_CLOSE, 0, 0);

						break;
					}

					switch (wParam)
					{
					case CommandID::SHOW_CONFIG_DIALOG:
						{
							// コンフィグダイアログを開きます。
							if (IDOK == showConfigDialog(hwnd))
							{
								// レイアウトを再計算します。
								calcAllLayout();

								// 設定が変更された可能性があるので、
								// メインウィンドウのメニューを更新します。
								updateMainMenu();
							}

							break;
						}
					case CommandID::IMPORT_LAYOUT:
						{
							// レイアウトファイルをインポートします。
							importLayout(hwnd);

							break;
						}
					case CommandID::EXPORT_LAYOUT:
						{
							// レイアウトファイルをエクスポートします。
							exportLayout(hwnd);

							break;
						}
					case CommandID::CREATE_SUB_WINDOW:
						{
							// サブウィンドウを新規作成します。
							createSubWindow(hwnd);

							break;
						}
					case SC_RESTORE:
					case SC_MINIMIZE: // 「patch.aul」用の処理です。
						{
							::SendMessage(hive.aviutlWindow, message, wParam, lParam);

							break;
						}
					}

					break;
				}
			case WM_INITMENUPOPUP:
				{
					if ((HMENU)wParam == subWindowMenu) updateSubWindowMenu();
					if ((HMENU)wParam == subProcessMenu) updateSubProcessMenu();

					break;
				}
			case WM_CREATE:
				{
					MY_TRACE(_T("MainWindow::onWndProc(WM_CREATE)\n"));

					hive.theme = ::OpenThemeData(hwnd, VSCLASS_WINDOW);
					MY_TRACE_HEX(hive.theme);

					subWindowMenu = ::CreatePopupMenu();
					MY_TRACE_HEX(subWindowMenu);

					subProcessMenu = ::CreatePopupMenu();
					MY_TRACE_HEX(subProcessMenu);

					break;
				}
			case WM_DESTROY:
				{
					MY_TRACE(_T("MainWindow::onWndProc(WM_DESTROY)\n"));

					::DestroyMenu(subProcessMenu), subProcessMenu = 0;
					::DestroyMenu(subWindowMenu), subWindowMenu = 0;
					::CloseThemeData(hive.theme), hive.theme = 0;

					break;
				}
			case Hive::WindowMessage::WM_POST_INIT: // AviUtlの初期化処理が終わったあとに通知されます。
				{
					// システムメニューに独自の項目を追加する。
					initSystemMenu();

					// 設定をファイルから読み込みます。
					loadConfig();

					break;
				}
			case Hive::WindowMessage::WM_LOAD_CONFIG: // 設定を読み込む必要があるときに通知されます。
				{
					// 設定をファイルから読み込みます。
					loadConfig();

					break;
				}
			case Hive::WindowMessage::WM_SAVE_CONFIG: // 設定を保存する必要があるときに通知されます。
				{
					// 設定をファイルに保存します。
					saveConfig();

					break;
				}
			}

			return __super::onWndProc(hwnd, message, wParam, lParam);
		}

		//
		// このクラスは設定を変更するためのダイアログです。
		//
		struct ConfigDialog : Tools::Dialog2
		{
			int doModal(HWND parent)
			{
				create(hive.instance, MAKEINTRESOURCE(IDD_CONFIG), parent);

				setUInt(IDC_FILL_COLOR, hive.fillColor);
				setUInt(IDC_BORDER_COLOR, hive.borderColor);
				setUInt(IDC_HOT_BORDER_COLOR, hive.hotBorderColor);
				setUInt(IDC_ACTIVE_CAPTION_COLOR, hive.activeCaptionColor);
				setUInt(IDC_ACTIVE_CAPTION_TEXT_COLOR, hive.activeCaptionTextColor);
				setUInt(IDC_INACTIVE_CAPTION_COLOR, hive.inactiveCaptionColor);
				setUInt(IDC_INACTIVE_CAPTION_TEXT_COLOR, hive.inactiveCaptionTextColor);
				setUInt(IDC_BORDER_WIDTH, Pane::borderWidth);
				setUInt(IDC_CAPTION_HEIGHT, Pane::captionHeight);
				setUInt(IDC_TAB_HEIGHT, Pane::tabHeight);
				setComboBox(IDC_TAB_MODE, Pane::tabMode, _T("タイトル"), _T("上"), _T("下"));
				setUInt(IDC_MENU_BREAK, hive.menuBreak);
				setCheck(IDC_USE_THEME, hive.useTheme);
				setCheck(IDC_FORCE_SCROLL, hive.forceScroll);
				setCheck(IDC_SHOW_PLAYER, hive.showPlayer);

				return doModal2(parent);
			}

			INT_PTR onDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override
			{
				switch (message)
				{
				case WM_COMMAND:
					{
						UINT id = LOWORD(wParam);

						switch (id)
						{
						case IDC_FILL_COLOR:
						case IDC_BORDER_COLOR:
						case IDC_HOT_BORDER_COLOR:
						case IDC_ACTIVE_CAPTION_COLOR:
						case IDC_ACTIVE_CAPTION_TEXT_COLOR:
						case IDC_INACTIVE_CAPTION_COLOR:
						case IDC_INACTIVE_CAPTION_TEXT_COLOR:
							{
								HWND control = (HWND)lParam;

								COLORREF color = ::GetDlgItemInt(hwnd, id, 0, FALSE);

								static COLORREF customColors[16] = {};
								CHOOSECOLOR cc { sizeof(cc) };
								cc.hwndOwner = hwnd;
								cc.lpCustColors = customColors;
								cc.rgbResult = color;
								cc.Flags = CC_RGBINIT | CC_FULLOPEN;
								if (!::ChooseColor(&cc)) return TRUE;

								color = cc.rgbResult;

								::SetDlgItemInt(hwnd, id, color, FALSE);
								::InvalidateRect(control, 0, FALSE);

								return TRUE;
							}
						}

						break;
					}
				case WM_DRAWITEM:
					{
						UINT id = wParam;

						switch (id)
						{
						case IDC_FILL_COLOR:
						case IDC_BORDER_COLOR:
						case IDC_HOT_BORDER_COLOR:
						case IDC_ACTIVE_CAPTION_COLOR:
						case IDC_ACTIVE_CAPTION_TEXT_COLOR:
						case IDC_INACTIVE_CAPTION_COLOR:
						case IDC_INACTIVE_CAPTION_TEXT_COLOR:
							{
								DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;

								COLORREF color = ::GetDlgItemInt(hwnd, id, 0, FALSE);

								HBRUSH brush = ::CreateSolidBrush(color);
								FillRect(dis->hDC, &dis->rcItem, brush);
								::DeleteObject(brush);

								return TRUE;
							}
						}

						break;
					}
				}

				return __super::onDlgProc(hwnd, message, wParam, lParam);
			}
		};
	}; inline auto mainWindow = std::make_shared<MainWindow>();
}
