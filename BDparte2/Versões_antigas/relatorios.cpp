#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <mariadb/mysql.h>

#pragma comment(lib, "comctl32.lib")

using namespace std;

wstring s_para_w(const string& s) {
    int sz = MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), NULL, 0);
    wstring w(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), &w[0], sz);
    return w;
}

class RelatorioGerenciador {
private:
    MYSQL* conexao;
public:
    RelatorioGerenciador() {
        conexao = mysql_init(NULL);
        mysql_real_connect(conexao, "localhost", "root", "@GoaT123", "livraria_bd", 3306, NULL, 0);
    }

    vector<wstring> estoqueCritico() {
        vector<wstring> res;
        // Lendo da VIEW
        mysql_query(conexao, "SELECT * FROM view_estoque_critico");
        MYSQL_RES* r = mysql_store_result(conexao);
        MYSQL_ROW l;
        while ((l = mysql_fetch_row(r))) {
            res.push_back(L"ID: " + s_para_w(l[0]) + L" - " + s_para_w(l[1]) + L" - Qtd: " + s_para_w(l[2]));
        }
        mysql_free_result(r);
        return res;
    }

    vector<wstring> rankingVendas() {
        vector<wstring> res;
        // Lendo da outra VIEW
        mysql_query(conexao, "SELECT * FROM view_relatorio_vendas");
        MYSQL_RES* r = mysql_store_result(conexao);
        MYSQL_ROW l;
        while ((l = mysql_fetch_row(r))) {
            res.push_back(L"Vendedor: " + s_para_w(l[0]) + L" - Vendas: " + s_para_w(l[1]) + L" - Total: R$ " + s_para_w(l[2] ? l[2] : "0.00"));
        }
        mysql_free_result(r);
        return res;
    }
};

RelatorioGerenciador db_rel;
HWND hListEstoque, hListVendas;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CREATE) {
        HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        CreateWindowW(L"STATIC", L"ALERTA: Estoque Baixo (< 5 unidades)", WS_CHILD | WS_VISIBLE, 20, 20, 300, 20, hwnd, NULL, NULL, NULL);
        hListEstoque = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 45, 400, 100, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L"DESEMPENHO: Vendas por Vendedor", WS_CHILD | WS_VISIBLE, 20, 160, 300, 20, hwnd, NULL, NULL, NULL);
        hListVendas = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 185, 400, 100, hwnd, NULL, NULL, NULL);

        // Preencher listas
        for (const auto& s : db_rel.estoqueCritico()) SendMessage(hListEstoque, LB_ADDSTRING, 0, (LPARAM)s.c_str());
        for (const auto& s : db_rel.rankingVendas()) SendMessage(hListVendas, LB_ADDSTRING, 0, (LPARAM)s.c_str());

        EnumChildWindows(hwnd, [](HWND c, LPARAM f) -> BOOL { SendMessage(c, WM_SETFONT, f, TRUE); return TRUE; }, (LPARAM)hFont);
    }
    if (uMsg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nC) {
    WNDCLASSW wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = L"RelatoriosP2"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowW(L"RelatoriosP2", L"Painel de BI - Relatórios e Views", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 100, 100, 460, 350, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nC);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}