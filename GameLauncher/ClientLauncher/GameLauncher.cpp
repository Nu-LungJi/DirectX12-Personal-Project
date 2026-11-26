#include "framework.h"
#include "resource.h"
#include "DEFINE.h"
#include "EngineLogger.h"
#include "GameManager.h"
#include "GameInstance.h"

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

    if (!ENGINE::EngineLogger::Initialize()) {
        OutputDebugStringW(L"Failed to initialize engine logger.\n");
        return EXIT_FAILURE;
    }

    struct LoggerScope final {
        ~LoggerScope() {
            if (!ENGINE::EngineLogger::Shutdown())
                OutputDebugStringW(L"Failed to flush engine logs.\n");
        }
    };

    LoggerScope LoggerLifetime;
    
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING) == 0) {
        MessageBoxW(nullptr, L"Failed to load window title.", L"Error", MB_OK);
        return EXIT_FAILURE;
    }

    if (LoadStringW(hInstance, IDC_GAMELAUNCHER, szWindowClass, MAX_LOADSTRING) == 0) {
        MessageBoxW(nullptr, L"Failed to load window class name.", L"Error", MB_OK);
        return EXIT_FAILURE;
    }

    if (MyRegisterClass(hInstance) == 0) {
        DWORD Error = GetLastError();

        MessageBoxW(nullptr, std::format(L"RegisterClassExW failed: {}", Error).c_str(), 
            L"Error", MB_OK);

        return EXIT_FAILURE;
    }

    BOOL Result = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (!Result) return FALSE;
    
    if (!InitInstance (hInstance, nCmdShow)) return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMELAUNCHER));

    MSG msg = {};
	BOOL SystemRunningMessage = TRUE;
    
    HRESULT RenderResult = S_OK;
    
    GameManager& GM = GameManager::Create();
    
    ENGINE::GraphicDeviceDesc GraphicDesc{};
    GraphicDesc.m_hWnd              = hWnd;
    GraphicDesc.m_iWindowWidth      = WindowWidth;
    GraphicDesc.m_iWindowHeight     = WindowHeight;
    GraphicDesc.m_iBackBufferCount  = BackBufferCount;
    GraphicDesc.m_bEnableVSync      = EnableVSync;
    GraphicDesc.m_bAllowTearing     = AllowTearing;

    const HRESULT InitializeResult = GM.Initialize(GraphicDesc);

    ENGINE::GameInstance::Get_Instance().Dump_DebugMessages();

    if (FAILED(InitializeResult)) {
        (void)ENGINE::EngineLogger::Error("Engine initialization failed: 0x{:08X}", static_cast<uint32_t>(InitializeResult));
        const HRESULT ShutdownResult = GM.ShutDown();

        if (FAILED(ShutdownResult)) 
            OutputDebugStringW(std::format(L"Engine shutdown failed. HRESULT: 0x{:08X}\n", static_cast<uint32_t>(ShutdownResult)).c_str());
        
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
        
        ENGINE::GameInstance::Get_Instance().Dump_DebugMessages();

        if (FAILED(RenderResult)) {
            (void)ENGINE::EngineLogger::Error(
                "Rendering failed: 0x{:08X}",
                static_cast<uint32_t>(RenderResult));

            break;
        }
        
        if (RenderResult == S_FALSE)
            WaitMessage();
    }

    const HRESULT ShutdownResult = GM.ShutDown();
    if (FAILED(ShutdownResult)) OutputDebugStringW(std::format(L"Engine shutdown failed. HRESULT: 0x{:08X}\n", static_cast<uint32_t>(ShutdownResult)).c_str());
    
    if (IsWindow(hWnd)) DestroyWindow(hWnd);

    return (FAILED(RenderResult) || FAILED(ShutdownResult)) ? EXIT_FAILURE : static_cast<int>(msg.wParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMELAUNCHER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
    wcex.lpszMenuName   = nullptr;
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

    //if (!hWnd)
    //    return FALSE;
    if (hWnd == nullptr)
    {
        DWORD Error = GetLastError();

        wchar_t Buffer[256]{};
        swprintf_s(
            Buffer,
            L"CreateWindowExW Failed! Error Code: %lu",
            Error
        );

        MessageBoxW(nullptr, Buffer, L"Window Error", MB_OK | MB_ICONERROR);
        return false;
    }
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
            const int wmId = LOWORD(wParam);
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
