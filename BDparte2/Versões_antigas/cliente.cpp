#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <mariadb/mysql.h>

// Habilita estilos visuais modernos
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf11df' language='*'\"")

using namespace std;

// --- Auxiliares de Conversão (Padrão do Projeto) ---
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

// --- Gerenciador de Clientes (Backend da Parte 2) ---
class ClienteGerenciador {
private:
    MYSQL* conexao;
public:
    ClienteGerenciador() {
        conexao = mysql_init(NULL);
        // Usando sua senha salva: @GoaT123
        if (!mysql_real_connect(conexao, "localhost", "root", "@GoaT123", "livraria_bd", 3306, NULL, 0)) {
            MessageBoxW(NULL, L"Erro ao conectar ao Banco de Dados!", L"Erro Crítico", MB_OK | MB_ICONERROR);
        }
    }
    ~ClienteGerenciador() { if (conexao) mysql_close(conexao); }

    bool inserir(wstring nome, wstring cidade, wstring time, bool anime) {
        if (nome.empty()) return false;
        string query = "INSERT INTO clientes (nome, cidade, time_coracao, assiste_one_piece) VALUES ('" + 
                       w_para_s(nome) + "', '" + w_para_s(cidade) + "', '" + w_para_s(time) + "', " + (anime ? "1" : "0") + ")";
        return mysql_query(conexao, query.c_str()) == 0;
    }

    vector<wstring> listar() {
        vector<wstring> res;
        // Puxamos todos os campos agora
        if (!mysql_query(conexao, "SELECT id, nome, cidade, time_coracao, assiste_one_piece FROM clientes")) {
            MYSQL_RES* r = mysql_store_result(conexao);
            if (r) {
                MYSQL_ROW l;
                while ((l = mysql_fetch_row(r))) {
                    // VERIFICAÇÃO DE SEGURANÇA (para não dar erro construction from null)
                    wstring id     = l[0] ? s_para_w(l[0]) : L"?";
                    wstring nome   = l[1] ? s_para_w(l[1]) : L"Sem Nome";
                    wstring cidade = l[2] ? s_para_w(l[2]) : L"---";
                    wstring time   = l[3] ? s_para_w(l[3]) : L"---";
                    int op_int     = l[4] ? atoi(l[4]) : 0;
                    wstring op_txt = (op_int == 1) ? L"Sim" : L"Não";
                    
                    // NOVA FORMATAÇÃO DESCRITIVA (Como solicitado)
                    wstring linha_formatada = L"ID: " + id + L" - Nome: " + nome + L" - Cid: " + cidade + L" - Time: " + time + L" - OP: " + op_txt;
                    res.push_back(linha_formatada);
                }
                mysql_free_result(r);
            }
        }
        return res;
    }
};

ClienteGerenciador db_cliente;
HFONT hFont;

// IDs dos Controles
#define ID_BTN_SALVAR_CLI 2001
HWND hEditNome, hEditCidade, hEditTime, hCheckAnime, hListClientes;

// Função para atualizar a lista visual
void AtualizarLista() {
    SendMessage(hListClientes, LB_RESETCONTENT, 0, 0);
    vector<wstring> clientes = db_cliente.listar();
    for (const auto& s : clientes) {
        SendMessage(hListClientes, LB_ADDSTRING, 0, (LPARAM)s.c_str());
    }
}

// Função para limpar os campos após cadastro
void LimparCampos() {
    SetWindowTextW(hEditNome, L"");
    SetWindowTextW(hEditCidade, L"");
    SetWindowTextW(hEditTime, L"");
    SendMessage(hCheckAnime, BM_SETCHECK, BST_UNCHECKED, 0);
    SetFocus(hEditNome); // Coloca o cursor de volta no nome
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            // --- Painel de Cadastro (Esquerda) ---
            CreateWindowW(L"STATIC", L"Nome do Cliente:", WS_CHILD | WS_VISIBLE, 20, 20, 120, 20, hwnd, NULL, NULL, NULL);
            hEditNome = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 150, 20, 200, 24, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Cidade:", WS_CHILD | WS_VISIBLE, 20, 55, 120, 20, hwnd, NULL, NULL, NULL);
            hEditCidade = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 150, 55, 200, 24, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Time do Coração:", WS_CHILD | WS_VISIBLE, 20, 90, 120, 20, hwnd, NULL, NULL, NULL);
            hEditTime = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 150, 90, 200, 24, hwnd, NULL, NULL, NULL);

            // Checkbox para One Piece
            hCheckAnime = CreateWindowW(L"BUTTON", L"Assiste One Piece?", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 150, 125, 200, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"BUTTON", L"Cadastrar Cliente", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 160, 330, 35, hwnd, (HMENU)ID_BTN_SALVAR_CLI, NULL, NULL);

            // --- Painel da Lista (Direita) ---
            CreateWindowW(L"STATIC", L"Clientes Cadastrados (Detalhes):", WS_CHILD | WS_VISIBLE, 380, 20, 250, 20, hwnd, NULL, NULL, NULL);
            // Aumentei a largura da ListBox para 550px para caber o texto descritivo
            hListClientes = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY, 380, 45, 550, 150, hwnd, NULL, NULL, NULL);

            // Aplicar Fonte e Estilos modernos em tudo
            EnumChildWindows(hwnd, [](HWND c, LPARAM f) -> BOOL { SendMessage(c, WM_SETFONT, f, TRUE); return TRUE; }, (LPARAM)hFont);
            
            AtualizarLista();
            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_BTN_SALVAR_CLI) {
                wchar_t n[256], c[100], t[100];
                GetWindowTextW(hEditNome, n, 256);
                GetWindowTextW(hEditCidade, c, 100);
                GetWindowTextW(hEditTime, t, 100);
                // Verifica o estado do Checkbox
                bool anime = (SendMessage(hCheckAnime, BM_GETCHECK, 0, 0) == BST_CHECKED);

                if (db_cliente.inserir(n, c, t, anime)) {
                    MessageBoxW(hwnd, L"Cliente cadastrado com sucesso!", L"Sucesso", MB_OK | MB_ICONINFORMATION);
                    AtualizarLista(); // Atualiza a lista com o novo formato
                    LimparCampos();   // Limpa a tela para o próximo
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
    // Inicializa controles modernos (necessário para o manifest)
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = L"ClientesP2V2"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);
    
    // Aumentei a largura da janela para 960px para caber a lista mais larga
    HWND hwnd = CreateWindowW(L"ClientesP2V2", L"Módulo de Clientes Melhorado - Parte 2", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 960, 260, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nC);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}