#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <mariadb/mysql.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf11df' language='*'\"")

using namespace std;

// --- Auxiliares ---
string w_para_s(const wstring& w) {
    int sz = WideCharToMultiByte(CP_UTF8, 0, &w[0], (int)w.size(), NULL, 0, NULL, NULL);
    string s(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, &w[0], (int)w.size(), &s[0], sz, NULL, NULL);
    return s;
}

// --- Gerenciador de Vendas ---
class VendaGerenciador {
private:
    MYSQL* conexao;
public:
    VendaGerenciador() {
        conexao = mysql_init(NULL);
        mysql_real_connect(conexao, "localhost", "root", "@GoaT123", "livraria_bd", 3306, NULL, 0);
    }
    ~VendaGerenciador() { if (conexao) mysql_close(conexao); }

    // Agora passamos estoqueAtual por referência para o botão saber quanto tem
    double calcularVenda(int idCli, int idLivro, int qtd, double& precoBase, int& estoqueAtual) {
        double desconto = 0.0;
        
        // Buscamos o preço e o ESTOQUE atual do livro
        string qLivro = "SELECT preco, estoque FROM livros WHERE id = " + to_string(idLivro);
        mysql_query(conexao, qLivro.c_str());
        MYSQL_RES* resL = mysql_store_result(conexao);
        MYSQL_ROW rowL = mysql_fetch_row(resL);
        if(rowL) {
            precoBase = atof(rowL[0]);
            estoqueAtual = atoi(rowL[1]); // Salva o estoque real
        } else {
            precoBase = 0.0;
            estoqueAtual = 0;
        }
        mysql_free_result(resL);

        // Busca dados do cliente (Descontos)
        string qCli = "SELECT cidade, time_coracao, assiste_one_piece FROM clientes WHERE id = " + to_string(idCli);
        mysql_query(conexao, qCli.c_str());
        MYSQL_RES* resC = mysql_store_result(conexao);
        MYSQL_ROW rowC = mysql_fetch_row(resC);
        
        if(rowC) {
            if(string(rowC[0]) == "Sousa") desconto += 0.10;
            if(string(rowC[1]) == "Flamengo") desconto += 0.05;
            if(atoi(rowC[2]) == 1) desconto += 0.10;
        }
        mysql_free_result(resC);

        return (precoBase * qtd) * (1.0 - desconto);
    }

    void finalizarVenda(int idC, int idV, int idL, int qtd, double total, string met) {
        string query = "CALL realizar_venda(" + to_string(idC) + "," + to_string(idV) + "," + 
                       to_string(idL) + "," + to_string(qtd) + "," + to_string(total) + ",'" + met + "')";
        mysql_query(conexao, query.c_str());
    }
};

VendaGerenciador db_venda;
HFONT hFont;
HWND hCli, hVend, hLivro, hQtd, hMetodo;
#define ID_BTN_VENDER 4001

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            CreateWindowW(L"STATIC", L"ID Cliente:", WS_CHILD | WS_VISIBLE, 20, 20, 120, 20, hwnd, NULL, NULL, NULL);
            hCli = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 150, 20, 60, 24, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"ID Vendedor:", WS_CHILD | WS_VISIBLE, 20, 55, 120, 20, hwnd, NULL, NULL, NULL);
            hVend = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 150, 55, 60, 24, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"ID Livro:", WS_CHILD | WS_VISIBLE, 20, 90, 120, 20, hwnd, NULL, NULL, NULL);
            hLivro = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 150, 90, 60, 24, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Quantidade:", WS_CHILD | WS_VISIBLE, 20, 125, 120, 20, hwnd, NULL, NULL, NULL);
            hQtd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1", WS_CHILD | WS_VISIBLE | ES_NUMBER, 150, 125, 60, 24, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Pagamento:", WS_CHILD | WS_VISIBLE, 20, 160, 120, 20, hwnd, NULL, NULL, NULL);
            hMetodo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 150, 160, 120, 100, hwnd, NULL, NULL, NULL);
            SendMessage(hMetodo, CB_ADDSTRING, 0, (LPARAM)L"Pix");
            SendMessage(hMetodo, CB_ADDSTRING, 0, (LPARAM)L"Cartão");
            SendMessage(hMetodo, CB_ADDSTRING, 0, (LPARAM)L"Berries");
            SendMessage(hMetodo, CB_SETCURSEL, 0, 0);

            CreateWindowW(L"BUTTON", L"Finalizar Venda com Descontos", WS_CHILD | WS_VISIBLE, 20, 210, 250, 40, hwnd, (HMENU)ID_BTN_VENDER, NULL, NULL);

            EnumChildWindows(hwnd, [](HWND c, LPARAM f) -> BOOL { SendMessage(c, WM_SETFONT, f, TRUE); return TRUE; }, (LPARAM)hFont);
            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_BTN_VENDER) {
                wchar_t c[10], v[10], l[10], q[10], m[50];
                GetWindowTextW(hCli, c, 10); GetWindowTextW(hVend, v, 10);
                GetWindowTextW(hLivro, l, 10); GetWindowTextW(hQtd, q, 10);
                SendMessage(hMetodo, CB_GETLBTEXT, SendMessage(hMetodo, CB_GETCURSEL, 0, 0), (LPARAM)m);

                if (wcslen(c) == 0 || wcslen(v) == 0 || wcslen(l) == 0) {
                    MessageBoxW(hwnd, L"Preencha todos os IDs!", L"Erro", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                int qtdDesejada = _wtoi(q);
                if (qtdDesejada <= 0) {
                    MessageBoxW(hwnd, L"A quantidade deve ser maior que zero!", L"Erro", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                double precoBase = 0;
                int estoqueAtual = 0; // Variável nova para guardar o estoque real
                
                double total = db_venda.calcularVenda(_wtoi(c), _wtoi(l), qtdDesejada, precoBase, estoqueAtual);
                
                if (precoBase == 0) {
                    MessageBoxW(hwnd, L"Livro não encontrado!", L"Erro", MB_OK | MB_ICONERROR);
                    return 0;
                }

                // 🔴 A TRAVA DE SEGURANÇA 🔴
                if (estoqueAtual < qtdDesejada) {
                    wstring msgErro = L"Estoque insuficiente!\nTemos apenas " + to_wstring(estoqueAtual) + L" unidade(s) disponível(is) para este livro.";
                    MessageBoxW(hwnd, msgErro.c_str(), L"Operação Negada", MB_OK | MB_ICONWARNING);
                    return 0; // Para a execução aqui e não faz a venda
                }

                wstring msg = L"Resumo do Pedido:\n\nPreço Original: R$ " + to_wstring(precoBase) + 
                              L"\nQuantidade: " + wstring(q) + 
                              L"\nTotal com Descontos: R$ " + to_wstring(total) + 
                              L"\n\nConfirmar Venda?";
                
                if(MessageBoxW(hwnd, msg.c_str(), L"Confirmar Checkout", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    db_venda.finalizarVenda(_wtoi(c), _wtoi(v), _wtoi(l), qtdDesejada, total, w_para_s(m));
                    MessageBoxW(hwnd, L"Venda concluída e estoque atualizado!", L"Sucesso", MB_OK | MB_ICONINFORMATION);
                }
            }
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nC) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = L"VendasP2"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);
    
    HWND hwnd = CreateWindowW(L"VendasP2", L"Sistema de Vendas - Checkout", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 310, 310, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nC);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}