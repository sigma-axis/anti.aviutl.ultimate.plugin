﻿#pragma once

namespace aut
{
	//
	// このクラスは他クラスから共通して使用される変数を保持します。
	//
	inline struct Hive
	{
		struct CommandID {
			struct SettingDialog {
				static const UINT ID_CREATE_CLONE			= 12020;
				static const UINT ID_CREATE_SAME_ABOVE		= 12021;
				static const UINT ID_CREATE_SAME_BELOW		= 12022;

				static const UINT ID_CUT_FILTER				= 12023;
				static const UINT ID_CUT_FILTER_ABOVE		= 12024;
				static const UINT ID_CUT_FILTER_BELOW		= 12025;
				static const UINT ID_COPY_FILTER			= 12026;
				static const UINT ID_COPY_FILTER_ABOVE		= 12027;
				static const UINT ID_COPY_FILTER_BELOW		= 12028;
				static const UINT ID_PASTE_FILTER			= 12029;
			};
		};

		AviUtl::FilterPlugin* fp = 0;
		AviUtlInternal auin;

		//
		// 指定されたコンフィグファイルをフルパスにして返します。
		//
		std::wstring getConfigFileName(LPCWSTR fileName) const
		{
			WCHAR path[MAX_PATH] = {};
			::GetModuleFileNameW(fp->dll_hinst, path, std::size(path));
			::PathRemoveExtensionW(path);
			::StringCbCatW(path, sizeof(path), L"Config");
			::PathAppendW(path, fileName);
			return path;
		}

		//
		// ハイブの初期化を実行します。
		//
		BOOL init(AviUtl::FilterPlugin* fp)
		{
			this->fp = fp;
			auin.initExEditAddress();

			return TRUE;
		}

		//
		// ハイブの後始末を実行します。
		//
		BOOL exit()
		{
			return TRUE;
		}
	} hive;
}
