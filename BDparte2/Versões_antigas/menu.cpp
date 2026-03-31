#include <windows.h>
#include <commctrl.h>
#include <shellapi.h> // Necessário para abrir outros programas

// Habilita o visual moderno do Windows
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf11df' language='*'\"")

// IDs dos Botões
#define ID_BTN_LIVROS     1001
#define ID_BTN_CLIENTES   1002
#define ID_BTN_VENDEDORES 1003
#define ID_BTN_VENDAS     1004
#define ID_BTN_RELATORIOS 1005

// Função auxiliar para abrir os executáveis
void AbrirModulo(HWND hwnd, LPCWSTR nomeExe) {
    HINSTANCE result = ShellExecuteW(hwnd, L"open", nomeExe, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        MessageBoxW(hwnd, L"Erro ao abrir o módulo! Verifique se o .exe está na mesma pasta que o menu.", L"Erro", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFontTitulo, hFontBotao;

    switch (uMsg) {
        case WM_CREATE: {
            // Criando as fontes (Título maior e em negrito)
            hFontTitulo = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            hFontBotao = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            // Label do Título
            HWND hTitulo = CreateWindowW(L"STATIC", L"LIVRARIA DO VICTOR - GESTÃO", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 20, 340, 30, hwnd, NULL, NULL, NULL);
            SendMessage(hTitulo, WM_SETFONT, (WPARAM)hFontTitulo, TRUE);

            // Botões
            HWND b1 = CreateWindowW(L"BUTTON", L"📦 Módulo de Livros (Estoque)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 70, 300, 40, hwnd, (HMENU)ID_BTN_LIVROS, NULL, NULL);
            HWND b2 = CreateWindowW(L"BUTTON", L"👥 Cadastro de Clientes", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 120, 300, 40, hwnd, (HMENU)ID_BTN_CLIENTES, NULL, NULL);
            HWND b3 = CreateWindowW(L"BUTTON", L"👔 Equipe de Vendedores", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 170, 300, 40, hwnd, (HMENU)ID_BTN_VENDEDORES, NULL, NULL);
            HWND b4 = CreateWindowW(L"BUTTON", L"🛒 Realizar Venda (Checkout)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 220, 300, 40, hwnd, (HMENU)ID_BTN_VENDAS, NULL, NULL);
            HWND b5 = CreateWindowW(L"BUTTON", L"📊 Relatórios e Views (BI)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 270, 300, 40, hwnd, (HMENU)ID_BTN_RELATORIOS, NULL, NULL);

            // Aplicando a fonte nos botões
            SendMessage(b1, WM_SETFONT, (WPARAM)hFontBotao, TRUE);
            SendMessage(b2, WM_SETFONT, (WPARAM)hFontBotao, TRUE);
            SendMessage(b3, WM_SETFONT, (WPARAM)hFontBotao, TRUE);
            SendMessage(b4, WM_SETFONT, (WPARAM)hFontBotao, TRUE);
            SendMessage(b5, WM_SETFONT, (WPARAM)hFontBotao, TRUE);
            
            return 0;
        }

        case WM_COMMAND: {
            // Verifica qual botão foi clicado e abre o arquivo .exe correspondente
            switch (LOWORD(wParam)) {
                case ID_BTN_LIVROS:     AbrirModulo(hwnd, L"livraria_v3.exe"); break;
                case ID_BTN_CLIENTES:   AbrirModulo(hwnd, L"cliente.exe"); break;
                case ID_BTN_VENDEDORES: AbrirModulo(hwnd, L"vendedores_v2.exe"); break;
                case ID_BTN_VENDAS:     AbrirModulo(hwnd, L"vendas.exe"); break;
                case ID_BTN_RELATORIOS: AbrirModulo(hwnd, L"relatorios.exe"); break;
            }
            return 0;
        }

        case WM_DESTROY: 
            PostQuitMessage(0); 
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nC) {
    INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {0}; 
    wc.lpfnWndProc = WindowProc; 
    wc.hInstance = hI; 
    wc.lpszClassName = L"MenuPrincipal"; 
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"MenuPrincipal", L"Menu Principal - BD I", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 400, 380, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nC);

    MSG msg; 
    while (GetMessage(&msg, NULL, 0, 0)) { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    }
    return 0;
}