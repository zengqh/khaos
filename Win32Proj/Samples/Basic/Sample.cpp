#include "stdafx.h"
#include "SampleFrame.h"
#include "resource.h"
#include <tchar.h>
#include <Windowsx.h>
#include <tlhelp32.h>

HINSTANCE   g_hInst = 0;
HWND        g_hWnd  = 0;
TCHAR       g_szTitle[256]       = _T("Sample");
TCHAR       g_szWindowClass[256] = _T("Sample");

bool        g_runApp = true;

ATOM             myRegisterClass(HINSTANCE hInstance);
BOOL             initInstance(HINSTANCE, int);
void             centerWindow();
LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
void             initSampleCreateContext( SampleCreateContext& scc );

DWORD getParentProcessID();
void getProcessName(WCHAR* szProcessName, int nLen, DWORD dwProcessID);
bool getProcessEntry(PROCESSENTRY32& pe, DWORD dwProcessID);

void checkSafe();

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    checkSafe();

    // create window
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	myRegisterClass(hInstance);

	if (!initInstance (hInstance, nCmdShow))
		return FALSE;

    centerWindow();

    // create sample
    createSampleFrame();

    SampleCreateContext scc;
    initSampleCreateContext( scc );

    if ( !g_sampleFrame->create( scc ) )
    {
        g_sampleFrame->release();
        return FALSE;
    }

    // run msg idle
    MSG msg = {0};

    while( msg.message != WM_QUIT )
    {
        if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else if ( g_runApp )
        {
            if ( !g_sampleFrame->runIdle() )
            {
                DestroyWindow( g_hWnd );
            }
        }
    }

    // clear
    g_sampleFrame->release();
	return (int) msg.wParam;
}

ATOM myRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= wndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SAMPLE));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= g_szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL initInstance(HINSTANCE hInstance, int nCmdShow)
{
   g_hInst = hInstance; // 将实例句柄存储在全局变量中

   g_hWnd = CreateWindow(g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!g_hWnd)
   {
      return FALSE;
   }

   ShowWindow(g_hWnd, nCmdShow);
   UpdateWindow(g_hWnd);

   return TRUE;
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		{
            PAINTSTRUCT ps;
            HDC hdc;
            hdc = BeginPaint(hWnd, &ps);
		    EndPaint(hWnd, &ps);
        }
		return 0;

    case WM_KEYUP:
        g_sampleFrame->onKeyUp( wParam );
        return 0;

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        {
            int xPos = GET_X_LPARAM(lParam); 
            int yPos = GET_Y_LPARAM(lParam); 

            int key = MK_LBUTTON;
            if ( message == WM_MBUTTONUP ) key = MK_MBUTTON;
            else if ( message == WM_RBUTTONUP ) key = MK_RBUTTON;

            g_sampleFrame->onMouseUp( key, xPos, yPos );
        }
        
        return 0;

	case WM_DESTROY:
        g_runApp = false;
        g_sampleFrame->destroy();
		PostQuitMessage(0);
		return 0;
	}
	
    return DefWindowProc(hWnd, message, wParam, lParam);
}

inline int getRectWidth( const RECT& rc )
{
    return rc.right - rc.left;
}

inline int getRectHeight( const RECT& rc )
{
    return rc.bottom - rc.top;
}

void centerWindow()
{
    RECT rectClient = { 0, 0, 1024, 768 };
    AdjustWindowRect( &rectClient, GetWindowLongPtr(g_hWnd, GWL_STYLE), FALSE );

    int width  = getRectWidth(rectClient);
    int height = getRectHeight(rectClient);

    RECT rectWork = {0};
    SystemParametersInfo( SPI_GETWORKAREA, 0, &rectWork, 0 );

    int rectWorkHeight = getRectHeight(rectWork);
    if ( height > rectWorkHeight )
    {
        width = width * rectWorkHeight / height;
        height = rectWorkHeight;
    }

    rectClient.left = rectWork.left + (getRectWidth(rectWork) - width) / 2;
    rectClient.top  = rectWork.top + (rectWorkHeight - height) / 2;

    MoveWindow( g_hWnd, rectClient.left, rectClient.top, width, height, FALSE );
}

void initSampleCreateContext( SampleCreateContext& scc )
{
    char exeName[MAX_PATH];
    GetModuleFileNameA( g_hInst, exeName, sizeof(exeName) );

    static char exePath[MAX_PATH];
    char* filePart;
    GetFullPathNameA( exeName, MAX_PATH, exePath, &filePart );
    *filePart = 0;

    RECT rcClient;
    GetClientRect( g_hWnd, &rcClient );

    scc.handleWindow = g_hWnd;
    scc.windowWidth  = getRectWidth(rcClient);
    scc.windowHeight = getRectHeight(rcClient);
    scc.exePath      = exePath;
}

//////////////////////////////////////////////////////////////////////////
bool getProcessEntry(PROCESSENTRY32& pe, DWORD dwProcessID)
{
    bool bFindProcess = false;
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    DWORD dwParentProcessID = -1;
    if( Process32First(h, &pe))
    {
        do
        {
            if (pe.th32ProcessID == dwProcessID) 
            {
                bFindProcess = true;
                break;
            }
        } while( Process32Next(h, &pe));
    }

    CloseHandle(h);

    return bFindProcess;
}

DWORD getParentProcessID()
{
    PROCESSENTRY32 pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32);
    DWORD dwProcessID = GetCurrentProcessId();
    if(getProcessEntry(pe, dwProcessID))
    {
        return pe.th32ParentProcessID;
    }
    return -1;
}

void getProcessName(WCHAR* szProcessName, int nLen, DWORD dwProcessID)
{
    PROCESSENTRY32 pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32);
    if(getProcessEntry(pe, dwProcessID))
    {
        swprintf(szProcessName, nLen-1, pe.szExeFile);
    }
}

void checkSafe()
{
    return;
#ifndef _DEBUG

    DWORD parentID = getParentProcessID();

    WCHAR name[1024] = {};
    getProcessName( name, 1024, parentID );

    bool x[100] = {};

    x[0] = 'e' == name[0];
    x[1] = 'x' == name[1];
    x[2] = 'p' == name[2];
    x[3] = 'l' == name[3];
    x[4] = 'o' == name[4];
    x[5] = 'r' == name[5];
    x[6] = 'e' == name[6];
    x[7] = 'r' == name[7];
    x[8] = '.' == name[8];
    x[9] = 'e' == name[9];
    x[10] = 'x' == name[10];
    x[11] = 'e' == name[11];

    for ( int i = 0; i < 12; ++i )
    {
        if ( !x[i] )
        {
            int * p = (int*)2;
            *p = 12345;
            exit(0);
            int * q = (int*)2;
            *q = 12345;
        }
    }


    if ( wcscmp( L"explorer.exe", name ) != 0 )
    {
        int * p = (int*)1;
        *p = 1;
        exit(0);
        int * q = (int*)1;
        *q = 1;
    }

#endif
}

