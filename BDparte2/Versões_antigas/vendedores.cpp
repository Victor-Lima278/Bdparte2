#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <mariadb/mysql.h>

// Habilita estilos modernos
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf11df' language='*'\"")

using namespace std;

// --- Auxiliares de Conversão ---
string w_para_s(const wstring& w) {
    if (w.empty()) return string();
    int sz = WideCharToMultiByte(CP_UTF8, 0, &w[0], (int)w.size(), NULL, 0, NULL, NULL);
    string s(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, &w[0], (int)w.size(), &s[0], sz, NULL, NULL);
    return s;
}

wstring s_para_w(const string& s) {
    if (s.empty()) return wstring();
    int sz = MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), NULL, 0);
    wstring w(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), &w[0], sz);
    return w;
}

// --- Backend ---
class VendedorGerenciador {
private:
    MYSQL* conexao;
public:
    VendedorGerenciador() {
        conexao = mysql_init(NULL);
        if (!mysql_real_connect(conexao, "localhost", "root", "@GoaT123", "livraria_bd", 3306, NULL, 0)) {
            MessageBoxW(NULL, L"Erro ao conectar ao Banco!", L"Erro", MB_OK | MB_ICONERROR);
        }
    }
    ~VendedorGerenciador() { if (conexao) mysql_close(conexao); }

    bool inserir(wstring nome) {
        if (nome.empty()) return false;
        string query = "INSERT INTO vendedores (nome) VALUES ('" + w_para_s(nome) + "')";
        return mysql_query(conexao, query.c_str()) == 0;
    }

    vector<wstring> listar() {
        vector<wstring> res;
        if (!mysql_query(conexao, "SELECT id, nome FROM vendedores")) {
            MYSQL_RES* r = mysql_store_result(conexao);
            if (r) {
                MYSQL_ROW l;
                while ((l = mysql_fetch_row(r))) {
                    wstring id = l[0] ? s_para_w(l[0]) : L"?";
                    wstring nome = l[1] ? s_para_w(l[1]) : L"Sem Nome";
                    // Formato descritivo solicitado
                    res.push_back(L"ID: " + id + L" - Nome: " + nome);
                }
                mysql_free_result(r);
            }
        }
        return res;
    }
};

VendedorGerenciador db_vendedor;
HFONT hFont;
#define ID_BTN_SALVAR_VEND 3001
HWND hEditNomeVend, hListVendedores;

void AtualizarLista() {
    SendMessage(hListVendedores, LB_RESETCONTENT, 0, 0);
    for (const auto& s : db_vendedor.listar()) SendMessage(hListVendedores, LB_ADDSTRING, 0, (LPARAM)s.c_str());
}

void LimparCampos() {
    SetWindowTextW(hEditNomeVend, L"");
    SetFocus(hEditNomeVend);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            // Painel de Cadastro
            CreateWindowW(L"STATIC", L"Nome do Vendedor:", WS_CHILD | WS_VISIBLE, 20, 20, 150, 20, hwnd, NULL, NULL, NULL);
            hEditNomeVend = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 20, 45, 250, 24, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"BUTTON", L"Registrar Vendedor", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 85, 250, 35, hwnd, (HMENU)ID_BTN_SALVAR_VEND, NULL, NULL);

            // Painel da Lista
            CreateWindowW(L"STATIC", L"Equipe de Vendas (Resumo):", WS_CHILD | WS_VISIBLE, 300, 20, 200, 20, hwnd, NULL, NULL, NULL);
            hListVendedores = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, 300, 45, 300, 150, hwnd, NULL, NULL, NULL);

            // Aplicar Fonte
            EnumChildWindows(hwnd, [](HWND c, LPARAM f) -> BOOL { SendMessage(c, WM_SETFONT, f, TRUE); return TRUE; }, (LPARAM)hFont);
            
            AtualizarLista();
            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_BTN_SALVAR_VEND) {
                wchar_t n[256];
                GetWindowTextW(hEditNomeVend, n, 256);
                if (db_vendedor.inserir(n)) {
                    MessageBoxW(hwnd, L"Vendedor cadastrado com sucesso!", L"Sucesso", MB_OK | MB_ICONINFORMATION);
                    LimparCampos();
                    AtualizarLista();
                } else {
                    MessageBoxW(hwnd, L"Erro ao cadastrar. O nome é obrigatório.", L"Erro", MB_OK | MB_ICONERROR);
                }
            }
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nC) {
    INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = L"VendP2Modern"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"VendP2Modern", L"Equipe de Vendas - Parte 2", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 640, 260, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nC);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}