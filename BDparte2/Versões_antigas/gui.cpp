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
class LivrariaGerenciador {
private:
    MYSQL* conexao;
public:
    LivrariaGerenciador() {
        conexao = mysql_init(NULL);
        if (!mysql_real_connect(conexao, "localhost", "root", "@GoaT123", "livraria_bd", 3306, NULL, 0)) {
            MessageBoxW(NULL, L"Erro ao conectar ao Banco!", L"Erro", MB_OK | MB_ICONERROR);
        }
    }
    ~LivrariaGerenciador() { if (conexao) mysql_close(conexao); }

    bool inserir(wstring t, wstring a, double p, int e, wstring tp, wstring g, wstring& erroMsg) {
        string query = "INSERT INTO livros (titulo, autor, preco, estoque, tipo, genero) VALUES ('" + 
                       w_para_s(t) + "', '" + w_para_s(a) + "', " + to_string(p) + ", " + to_string(e) + 
                       ", '" + w_para_s(tp) + "', '" + w_para_s(g) + "')";
        
        if (mysql_query(conexao, query.c_str()) == 0) return true;
        erroMsg = s_para_w(mysql_error(conexao));
        return false;
    }

    bool atualizar(int id, wstring t, wstring a, double p, wstring tp, wstring g, wstring& erroMsg) {
        string query = "UPDATE livros SET titulo = '" + w_para_s(t) + 
                       "', autor = '" + w_para_s(a) + 
                       "', preco = " + to_string(p) + 
                       ", tipo = '" + w_para_s(tp) + 
                       "', genero = '" + w_para_s(g) + 
                       "' WHERE id = " + to_string(id);
                       
        if (mysql_query(conexao, query.c_str()) == 0) {
            return mysql_affected_rows(conexao) > 0;
        }
        erroMsg = s_para_w(mysql_error(conexao));
        return false;
    }

    bool buscarPorId(int id, wstring& t, wstring& a, wstring& p, wstring& tp, wstring& g) {
        string q = "SELECT titulo, autor, preco, tipo, genero FROM livros WHERE id = " + to_string(id);
        if (mysql_query(conexao, q.c_str()) == 0) {
            MYSQL_RES* r = mysql_store_result(conexao);
            if (r) {
                MYSQL_ROW row = mysql_fetch_row(r);
                if (row) {
                    t = row[0] ? s_para_w(row[0]) : L"";
                    a = row[1] ? s_para_w(row[1]) : L"";
                    p = row[2] ? s_para_w(row[2]) : L"";
                    tp = row[3] ? s_para_w(row[3]) : L"";
                    g = row[4] ? s_para_w(row[4]) : L"";
                    mysql_free_result(r);
                    return true;
                }
                mysql_free_result(r);
            }
        }
        return false;
    }

    // A MÁGICA ACONTECE AQUI: Nova lógica de busca global
    vector<wstring> listar(wstring filtro = L"") {
        vector<wstring> res;
        string q = "SELECT id, titulo, autor, tipo, genero, preco, estoque FROM livros";
        
        if (!filtro.empty()) {
            string f = w_para_s(filtro);
            // Agora pesquisa em Título, Autor, Tipo OU Gênero
            q += " WHERE titulo LIKE '%" + f + "%' OR autor LIKE '%" + f + "%' OR tipo LIKE '%" + f + "%' OR genero LIKE '%" + f + "%'";
        }
        
        if (!mysql_query(conexao, q.c_str())) {
            MYSQL_RES* r = mysql_store_result(conexao);
            if (r) {
                MYSQL_ROW l;
                while ((l = mysql_fetch_row(r))) {
                    wstring id = l[0] ? s_para_w(l[0]) : L"?";
                    wstring tit = l[1] ? s_para_w(l[1]) : L"---";
                    wstring aut = l[2] ? s_para_w(l[2]) : L"---";
                    wstring tp = l[3] ? s_para_w(l[3]) : L"---";
                    wstring gen = l[4] ? s_para_w(l[4]) : L"---";
                    wstring prc = l[5] ? s_para_w(l[5]) : L"0.00";
                    wstring est = l[6] ? s_para_w(l[6]) : L"0";

                    wstring item = L"ID: " + id + L" - Título: " + tit + L" - Autor: " + aut + 
                                   L" - Tipo: " + tp + L" - Gênero: " + gen + 
                                   L" - R$: " + prc + L" - Qtd: " + est;
                    res.push_back(item);
                }
                mysql_free_result(r);
            }
        }
        return res;
    }

    int remover(int id) {
        string q = "DELETE FROM livros WHERE id = " + to_string(id);
        if (mysql_query(conexao, q.c_str()) == 0) {
            if (mysql_affected_rows(conexao) > 0) return 1; 
            return 0; 
        } else {
            if (mysql_errno(conexao) == 1451) return -1;
            return 0; 
        }
    }

    bool adicionarEstoque(int id, int qtd) {
        if (qtd <= 0) return false;
        string q = "UPDATE livros SET estoque = estoque + " + to_string(qtd) + " WHERE id = " + to_string(id);
        return mysql_query(conexao, q.c_str()) == 0 && mysql_affected_rows(conexao) > 0;
    }

    wstring obterRelatorio() {
        if (!mysql_query(conexao, "SELECT COUNT(*), SUM(estoque), SUM(preco * estoque) FROM livros")) {
            MYSQL_RES* r = mysql_store_result(conexao);
            if (r) {
                MYSQL_ROW l = mysql_fetch_row(r);
                wstring res = L"Total de Títulos Diferentes: " + s_para_w(l[0]?l[0]:"0") + L"\n" +
                              L"Total de Exemplares Físicos: " + s_para_w(l[1]?l[1]:"0") + L"\n" +
                              L"Valor Total em Estoque: R$ " + s_para_w(l[2]?l[2]:"0.00");
                mysql_free_result(r);
                return res;
            }
        }
        return L"Erro.";
    }
};

LivrariaGerenciador sistema;
HFONT hFont;

#define ID_BTN_INSERIR     1001
#define ID_BTN_REMOVER     1002
#define ID_BTN_PESQUISAR   1003
#define ID_BTN_RELATORIO   1004
#define ID_BTN_ADD_ESTOQUE 1005
#define ID_BTN_CARREGAR    1006
#define ID_BTN_ATUALIZAR   1007

HWND hEditT, hEditA, hEditP, hEditE, hComboTipo, hComboGen, hListBox, hEditBusca, hEditIdAcao, hEditQtdAcao;

void Atualizar(wstring f = L"") {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    for (const auto& s : sistema.listar(f)) SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)s.c_str());
}

void LimparCampos() {
    SetWindowTextW(hEditT, L"");
    SetWindowTextW(hEditA, L"");
    SetWindowTextW(hEditP, L"");
    SetWindowTextW(hEditE, L"");
    SendMessage(hComboTipo, CB_SETCURSEL, 0, 0);
    SendMessage(hComboGen, CB_SETCURSEL, 0, 0);
    SetFocus(hEditT);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            // --- Painel Esquerdo (Formulário) ---
            CreateWindowW(L"STATIC", L"Título:", WS_CHILD | WS_VISIBLE, 20, 20, 60, 20, hwnd, NULL, NULL, NULL);
            hEditT = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 85, 20, 180, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowW(L"STATIC", L"Autor:", WS_CHILD | WS_VISIBLE, 20, 55, 60, 20, hwnd, NULL, NULL, NULL);
            hEditA = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 85, 55, 180, 24, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"Tipo:", WS_CHILD | WS_VISIBLE, 20, 90, 60, 20, hwnd, NULL, NULL, NULL);
            hComboTipo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 85, 90, 180, 100, hwnd, NULL, NULL, NULL);
            SendMessage(hComboTipo, CB_ADDSTRING, 0, (LPARAM)L"Livro");
            SendMessage(hComboTipo, CB_ADDSTRING, 0, (LPARAM)L"Mangá");
            SendMessage(hComboTipo, CB_SETCURSEL, 0, 0);

            CreateWindowW(L"STATIC", L"Gênero:", WS_CHILD | WS_VISIBLE, 20, 125, 60, 20, hwnd, NULL, NULL, NULL);
            hComboGen = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 85, 125, 180, 100, hwnd, NULL, NULL, NULL);
            const wchar_t* gens[] = { L"Aventura", L"Ação", L"Romance", L"Terror", L"Ficção", L"Comédia" };
            for(int i=0; i<6; i++) SendMessage(hComboGen, CB_ADDSTRING, 0, (LPARAM)gens[i]);
            SendMessage(hComboGen, CB_SETCURSEL, 0, 0);

            CreateWindowW(L"STATIC", L"Preço:", WS_CHILD | WS_VISIBLE, 20, 160, 60, 20, hwnd, NULL, NULL, NULL);
            hEditP = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 85, 160, 70, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowW(L"STATIC", L"Qtd Inicial:", WS_CHILD | WS_VISIBLE, 165, 160, 75, 20, hwnd, NULL, NULL, NULL);
            hEditE = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER, 240, 160, 45, 24, hwnd, NULL, NULL, NULL);

            // Botões do Painel Esquerdo
            CreateWindowW(L"BUTTON", L"Cadastrar Novo Título", WS_CHILD | WS_VISIBLE, 20, 195, 265, 30, hwnd, (HMENU)ID_BTN_INSERIR, NULL, NULL);
            CreateWindowW(L"BUTTON", L"Salvar Alterações (Edição)", WS_CHILD | WS_VISIBLE, 20, 230, 265, 30, hwnd, (HMENU)ID_BTN_ATUALIZAR, NULL, NULL);
            CreateWindowW(L"BUTTON", L"Gerar Relatório de Patrimônio", WS_CHILD | WS_VISIBLE, 20, 265, 265, 30, hwnd, (HMENU)ID_BTN_RELATORIO, NULL, NULL);

            // --- Painel Direito (Gerenciamento e Busca) ---
            
            // Alterado para "Pesquisar:"
            CreateWindowW(L"STATIC", L"Pesquisar:", WS_CHILD | WS_VISIBLE, 320, 22, 90, 20, hwnd, NULL, NULL, NULL);
            hEditBusca = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 400, 20, 220, 24, hwnd, NULL, NULL, NULL);
            CreateWindowW(L"BUTTON", L"Buscar", WS_CHILD | WS_VISIBLE, 630, 19, 80, 26, hwnd, (HMENU)ID_BTN_PESQUISAR, NULL, NULL);
            
            // Linha de Ações baseadas no ID
            CreateWindowW(L"STATIC", L"ID do Livro:", WS_CHILD | WS_VISIBLE, 320, 57, 90, 20, hwnd, NULL, NULL, NULL);
            hEditIdAcao = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER, 415, 55, 50, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowW(L"BUTTON", L"Carregar", WS_CHILD | WS_VISIBLE, 470, 54, 80, 26, hwnd, (HMENU)ID_BTN_CARREGAR, NULL, NULL);
            CreateWindowW(L"BUTTON", L"Remover", WS_CHILD | WS_VISIBLE, 555, 54, 80, 26, hwnd, (HMENU)ID_BTN_REMOVER, NULL, NULL);

            CreateWindowW(L"STATIC", L"+ Qtd:", WS_CHILD | WS_VISIBLE, 650, 57, 45, 20, hwnd, NULL, NULL, NULL);
            hEditQtdAcao = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER, 695, 55, 50, 24, hwnd, NULL, NULL, NULL);
            CreateWindowW(L"BUTTON", L"Add", WS_CHILD | WS_VISIBLE, 750, 54, 60, 26, hwnd, (HMENU)ID_BTN_ADD_ESTOQUE, NULL, NULL);

            hListBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL, 320, 95, 630, 200, hwnd, NULL, NULL, NULL);

            EnumChildWindows(hwnd, [](HWND c, LPARAM f) -> BOOL { SendMessage(c, WM_SETFONT, f, TRUE); return TRUE; }, (LPARAM)hFont);
            Atualizar();
            return 0;
        }

        case WM_COMMAND: {
            
            if (LOWORD(wParam) == ID_BTN_INSERIR) {
                wchar_t t[256], a[256], p[20], e[20], tp[50], g[50];
                GetWindowTextW(hEditT, t, 256); GetWindowTextW(hEditA, a, 256);
                GetWindowTextW(hEditP, p, 20); GetWindowTextW(hEditE, e, 20);
                
                if (wcslen(t) == 0 || wcslen(a) == 0 || wcslen(p) == 0 || wcslen(e) == 0) {
                    MessageBoxW(hwnd, L"Preencha todos os campos antes de cadastrar!", L"Aviso", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                SendMessage(hComboTipo, CB_GETLBTEXT, SendMessage(hComboTipo, CB_GETCURSEL, 0, 0), (LPARAM)tp);
                SendMessage(hComboGen, CB_GETLBTEXT, SendMessage(hComboGen, CB_GETCURSEL, 0, 0), (LPARAM)g);
                
                wstring msgErro;
                if (sistema.inserir(t, a, _wtof(p), _wtoi(e), tp, g, msgErro)) {
                    MessageBoxW(hwnd, L"Novo livro cadastrado com sucesso!", L"Sucesso", MB_OK | MB_ICONINFORMATION);
                    Atualizar();
                    LimparCampos();
                } else {
                    MessageBoxW(hwnd, (L"Erro ao cadastrar:\n" + msgErro).c_str(), L"Erro no Banco", MB_OK | MB_ICONERROR);
                }
            }
            
            if (LOWORD(wParam) == ID_BTN_CARREGAR) {
                wchar_t idB[20]; GetWindowTextW(hEditIdAcao, idB, 20);
                if (wcslen(idB) > 0) {
                    wstring t, a, p, tp, g;
                    if (sistema.buscarPorId(_wtoi(idB), t, a, p, tp, g)) {
                        SetWindowTextW(hEditT, t.c_str());
                        SetWindowTextW(hEditA, a.c_str());
                        SetWindowTextW(hEditP, p.c_str());
                        
                        SendMessage(hComboTipo, CB_SELECTSTRING, -1, (LPARAM)tp.c_str());
                        SendMessage(hComboGen, CB_SELECTSTRING, -1, (LPARAM)g.c_str());
                        
                        SetWindowTextW(hEditE, L""); 
                        MessageBoxW(hwnd, L"Dados carregados! Altere os campos à esquerda e clique em 'Salvar Alterações'.", L"Modo Edição", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxW(hwnd, L"Livro não encontrado com este ID.", L"Erro", MB_OK | MB_ICONERROR);
                    }
                } else {
                    MessageBoxW(hwnd, L"Digite o ID do livro e clique em Carregar.", L"Aviso", MB_OK | MB_ICONWARNING);
                }
            }

            if (LOWORD(wParam) == ID_BTN_ATUALIZAR) {
                wchar_t idB[20]; GetWindowTextW(hEditIdAcao, idB, 20);
                if (wcslen(idB) == 0) {
                    MessageBoxW(hwnd, L"Você precisa carregar o ID de um livro primeiro para atualizar!", L"Aviso", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                wchar_t t[256], a[256], p[20], tp[50], g[50];
                GetWindowTextW(hEditT, t, 256); GetWindowTextW(hEditA, a, 256); GetWindowTextW(hEditP, p, 20);
                SendMessage(hComboTipo, CB_GETLBTEXT, SendMessage(hComboTipo, CB_GETCURSEL, 0, 0), (LPARAM)tp);
                SendMessage(hComboGen, CB_GETLBTEXT, SendMessage(hComboGen, CB_GETCURSEL, 0, 0), (LPARAM)g);

                if (wcslen(t) == 0 || wcslen(a) == 0 || wcslen(p) == 0) {
                    MessageBoxW(hwnd, L"Título, Autor e Preço são obrigatórios!", L"Aviso", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                wstring msgErro;
                if (sistema.atualizar(_wtoi(idB), t, a, _wtof(p), tp, g, msgErro)) {
                    MessageBoxW(hwnd, L"Livro atualizado com sucesso!", L"Sucesso", MB_OK | MB_ICONINFORMATION);
                    Atualizar();
                    LimparCampos();
                    SetWindowTextW(hEditIdAcao, L"");
                } else {
                    MessageBoxW(hwnd, L"Nenhuma alteração foi feita ou ocorreu um erro.", L"Aviso", MB_OK | MB_ICONWARNING);
                }
            }
            
            if (LOWORD(wParam) == ID_BTN_PESQUISAR) {
                wchar_t b[256]; GetWindowTextW(hEditBusca, b, 256);
                Atualizar(b);
            }
            
            if (LOWORD(wParam) == ID_BTN_ADD_ESTOQUE) {
                wchar_t idW[20], qtdW[20];
                GetWindowTextW(hEditIdAcao, idW, 20); GetWindowTextW(hEditQtdAcao, qtdW, 20);
                if (wcslen(idW) > 0 && wcslen(qtdW) > 0) {
                    if (sistema.adicionarEstoque(_wtoi(idW), _wtoi(qtdW))) {
                        MessageBoxW(hwnd, L"Estoque atualizado!", L"Sucesso", MB_OK | MB_ICONINFORMATION);
                        SetWindowTextW(hEditIdAcao, L""); SetWindowTextW(hEditQtdAcao, L"");
                        Atualizar();
                    } else {
                        MessageBoxW(hwnd, L"Erro ao adicionar estoque.", L"Erro", MB_OK | MB_ICONERROR);
                    }
                }
            }
            
            if (LOWORD(wParam) == ID_BTN_REMOVER) {
                wchar_t idB[20]; GetWindowTextW(hEditIdAcao, idB, 20);
                if (wcslen(idB) > 0) {
                    if (MessageBoxW(hwnd, L"Tem certeza que deseja remover este livro?", L"Atenção", MB_YESNO | MB_ICONWARNING) == IDYES) {
                        int status = sistema.remover(_wtoi(idB)); 
                        if (status == 1) { MessageBoxW(hwnd, L"Removido!", L"Sucesso", MB_OK); SetWindowTextW(hEditIdAcao, L""); Atualizar(); }
                        else if (status == -1) { MessageBoxW(hwnd, L"BLOQUEADO: Possui histórico de vendas.", L"Proteção Ativa", MB_OK | MB_ICONERROR); }
                    }
                }
            }
            
            if (LOWORD(wParam) == ID_BTN_RELATORIO) {
                MessageBoxW(hwnd, sistema.obterRelatorio().c_str(), L"Relatório Patrimonial", MB_OK | MB_ICONINFORMATION);
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
    WNDCLASSW wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = L"LivrariaV3"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowW(L"LivrariaV3", L"Gestão de Estoque Detalhada", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 380, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nC);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}