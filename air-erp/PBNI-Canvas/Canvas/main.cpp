//****************************************************************************
#pragma warning(disable : 4786)

#include <windows.h>
#include <pbext.h>
#include <richedit.h>
#include <commctrl.h>
#include <math.h>

#include <string>
using std::string;

HMODULE g_dll_hModule = 0;

PBXEXPORT LPCTSTR PBXCALL PBX_GetDescription()
{
	static const TCHAR desc[] = {
		"class u_canvas from userobject\n"
		"event int onpaint(ulong hdc)\n"
		"end class\n" 
	};

  return desc;
}

class CVisualExt : public IPBX_VisualObject
{
	static TCHAR s_className[];

	IPB_Session* d_session;
	pbobject	d_pbobj;
	HWND		d_hwnd;
	string		d_text;
	bool		d_buffer;

public:
	static HDC			memdc;

	CVisualExt(IPB_Session* session, pbobject pbobj)
		: 
		d_session(session), 
		d_pbobj(pbobj), 
		d_hwnd(NULL),
		d_text("Visual Extension")
	{
		
	}

	~CVisualExt()
	{
	}

	LPCTSTR GetWindowClassName();

	HWND CreateControl
	(
		DWORD dwExStyle,      // extended window style
		LPCTSTR lpWindowName, // window name
		DWORD dwStyle,        // window style
		int x,                // horizontal position of window
		int y,                // vertical position of window
		int nWidth,           // window width
		int nHeight,          // window height
		HWND hWndParent,      // handle to parent or owner window
		HINSTANCE hInstance   // handle to application instance
	);

	PBXRESULT Invoke
		(
		IPB_Session	*session, 
		pbobject	obj, 
		pbmethodID	mid,
		PBCallInfo	*ci
		);

	void Destroy()
	{
		delete this;
		DestroyWindow(d_hwnd);
	}

	HDC GetMemDC()
	{
		return memdc;
	}

	void TriggerEvent(LPCTSTR eventName);

	static void RegisterClass();
	static void UnregisterClass();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam); 
};

TCHAR CVisualExt::s_className[] = "PBVisualExt";
HDC CVisualExt::memdc = 0;

LPCTSTR CVisualExt::GetWindowClassName()
{
	return s_className;
}

HWND CVisualExt::CreateControl
(
	DWORD dwExStyle,      // extended window style
	LPCTSTR lpWindowName, // window name
	DWORD dwStyle,        // window style
	int x,                // horizontal position of window
	int y,                // vertical position of window
	int nWidth,           // window width
	int nHeight,          // window height
	HWND hWndParent,      // handle to parent or owner window
	HINSTANCE hInstance   // handle to application instance
)
{
	d_hwnd = CreateWindowEx(dwExStyle, s_className, lpWindowName, dwStyle,
		x, y, nWidth, nHeight, hWndParent, NULL, hInstance, NULL);

	::SetWindowLong(d_hwnd, GWL_USERDATA, (LONG)this);

	return d_hwnd;
}

PBXRESULT CVisualExt::Invoke
	(
	IPB_Session	*session, 
	pbobject	obj, 
	pbmethodID	mid,
	PBCallInfo	*ci
	)
{
	//switch(mid)
	//{
	//case 1:
	//	return PBX_FAIL;
	//default:
	//	return PBX_FAIL;
	//}

	return PBX_OK;
}

void CVisualExt::TriggerEvent(LPCTSTR eventName)
{
	pbclass clz = d_session->GetClass(d_pbobj);
	pbmethodID mid = d_session->GetMethodID(clz, eventName, PBRT_EVENT, "");
	pbulong lpb = (pbulong)memdc;

	PBCallInfo ci;
	d_session->InitCallInfo(clz, mid, &ci);

	pbint cnt = ci.pArgs->GetCount();
	
	ci.pArgs->GetAt(0)->SetUlong(lpb);

	d_session->TriggerEvent(d_pbobj, mid, &ci);
	d_session->FreeCallInfo(&ci);

	//ci.returnValue->SetInt(0);
}

void CVisualExt::RegisterClass()
{
	WNDCLASS wndclass;

	wndclass.style           = CS_GLOBALCLASS | CS_DBLCLKS;
	wndclass.lpfnWndProc     = WindowProc;
	wndclass.cbClsExtra      = 0;
	wndclass.cbWndExtra      = 0;
	wndclass.hInstance       = g_dll_hModule;
	wndclass.hIcon           = NULL;
	wndclass.hCursor         = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground   = (HBRUSH) (COLOR_WINDOW + 1);
	wndclass.lpszMenuName    = NULL;
	wndclass.lpszClassName   = s_className;

	::RegisterClass (&wndclass);
}

void CVisualExt::UnregisterClass()
{
	::UnregisterClass(s_className, g_dll_hModule);
}

LRESULT CALLBACK CVisualExt::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CVisualExt* ext = (CVisualExt*)::GetWindowLong(hwnd, GWL_USERDATA);

	switch(uMsg)
	{
		case WM_CREATE:
			return 0;

		case WM_SIZE:
			return 0;

		case WM_COMMAND:
			return 0;

		case WM_ERASEBKGND:
			return 1;

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);

				RECT rc;
				GetClientRect(hwnd, &rc);

				HDC lmemdc = CreateCompatibleDC(hdc);
				HBITMAP hmembmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
				HBITMAP oldbmp = (HBITMAP)SelectObject(lmemdc, hmembmp);

				memdc = lmemdc;

				ext->TriggerEvent("onpaint");

				BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, lmemdc, 0, 0, SRCCOPY);
				
				SelectObject(lmemdc, oldbmp);
				DeleteObject(hmembmp);
				DeleteDC(lmemdc);
				EndPaint(hwnd, &ps);
				return 0;
			}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

PBXEXPORT PBXRESULT PBXCALL PBX_CreateVisualObject
(
	IPB_Session*			pbsession, 
	pbobject				pbobj,
	LPCTSTR					className,		
	IPBX_VisualObject	**obj
)
{
	PBXRESULT result = PBX_OK;

	string cn(className);
	if (cn.compare("u_canvas") == 0)
	{
		*obj = new CVisualExt(pbsession, pbobj);
	}
	else
	{
		*obj = NULL;
		result = PBX_FAIL;
	}

	return PBX_OK;
};

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved
					 )
{
	g_dll_hModule = HMODULE(hModule);

    switch (reasonForCall)
	{
		case DLL_PROCESS_ATTACH:
			CVisualExt::RegisterClass();
			break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			CVisualExt::UnregisterClass();
			break;
    }
    return TRUE;
}