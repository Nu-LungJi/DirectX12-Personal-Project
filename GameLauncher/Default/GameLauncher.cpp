#include "PCH.h"
#include "framework.h"
#include "PCH.h"
#include "DEFINE.h"
#include "GameLauncher.h"
#include "GameManager.h"

constexpr int MAX_LOADSTRING = 100;

HINSTANCE   hInst; HWND hWnd;
WCHAR       szTitle[MAX_LOADSTRING];
WCHAR       szWindowClass[MAX_LOADSTRING];

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {

#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GAMELAUNCHER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    BOOL Result = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (!Result) return FALSE;
    
    if (!InitInstance (hInstance, nCmdShow)) return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMELAUNCHER));

    MSG msg = {};
	BOOL SystemRunningMessage = TRUE;
    
    HRESULT RenderResult = S_OK;
    
    GameManager& GM = GameManager::Create();
    
    ENGINE::GraphicDeviceDesc GraphicDesc{};
    GraphicDesc.WindowHandle    = hWnd;
    GraphicDesc.WindowWidth     = WindowWidth;
    GraphicDesc.WindowHeight    = WindowHeight;
    GraphicDesc.BackBufferCount = 3;
    GraphicDesc.EnableVSync     = true;
    GraphicDesc.AllowTearing    = true;

    if (FAILED(GM.Initialize(GraphicDesc)))
    {
        DestroyWindow(hWnd);
        return EXIT_FAILURE;
    }

    while (SystemRunningMessage) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                SystemRunningMessage = FALSE;
                break;
            }
            else {
                if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
		
        if (!SystemRunningMessage) break;
        
        GM.Update();
        GM.LateUpdate();
        RenderResult = GM.Render();
        
        if (FAILED(RenderResult)) {
            OutputDebugStringW(L"Rendering failed.\n");
            break;
        }
        
        if (RenderResult == S_FALSE)
            WaitMessage();
    }

    if (IsWindow(hWnd))
        DestroyWindow(hWnd);

    return FAILED(RenderResult) ? EXIT_FAILURE : static_cast<int>(msg.wParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMELAUNCHER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    constexpr DWORD Style = WS_OVERLAPPEDWINDOW;
    constexpr DWORD ExStyle = 0;

    RECT Rect{
        0, 0,
        static_cast<LONG>(WindowWidth),
        static_cast<LONG>(WindowHeight)
    };

    if (!AdjustWindowRectExForDpi(&Rect, Style, FALSE, ExStyle, GetDpiForSystem()))
        return FALSE;
    
    hWnd = CreateWindowExW(
        ExStyle,
        szWindowClass,
        szTitle,
        Style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        Rect.right - Rect.left,
        Rect.bottom - Rect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_KEYDOWN: {
            switch (wParam) {
            case VK_ESCAPE:
                PostQuitMessage(0);
                break;
            default: ;
            }
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                ENGINE::GameInstance::Get_Instance().Request_TimeReset();
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
        {
            const uint32_t Width = wParam == SIZE_MINIMIZED ? 0u : static_cast<uint32_t>(LOWORD(lParam));
            const uint32_t Height = wParam == SIZE_MINIMIZED ? 0u : static_cast<uint32_t>(HIWORD(lParam));

            ENGINE::GameInstance::Get_Instance().Request_Resize(Width, Height);
            return 0;
        }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SETFOCUS:
        // 게임 창이 키보드 포커스를 얻음
        ENGINE::GameInstance::Get_Instance().Set_InputFocused(true);
        return 0;

    case WM_KILLFOCUS:
        // 다른 창이나 대화상자로 키보드 포커스가 이동함
        ENGINE::GameInstance::Get_Instance().Set_InputFocused(false);
        return 0;

    case WM_ACTIVATEAPP:
        // Alt+Tab 등으로 다른 프로그램이 활성화됨
        if (!wParam)
        {
            ENGINE::GameInstance::Get_Instance().Set_InputFocused(false);
        }
        break;
    case WM_EXITSIZEMOVE:
        ENGINE::GameInstance::Get_Instance().Request_TimeReset();
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    default: ;
    }
    return (INT_PTR)FALSE;
}
