#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <utility>
#include <mariadb/mysql.h>
#include <cwchar>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf11df' language='*'\"")

using namespace std;

string w_para_s(const wstring& w) {
    if (w.empty()) return string();
    int sz = WideCharToMultiByte(CP_UTF8, 0, &w[0], (int)w.size(), NULL, 0, NULL, NULL);
    string s(sz, 0); WideCharToMultiByte(CP_UTF8, 0, &w[0], (int)w.size(), &s[0], sz, NULL, NULL); return s;
}

wstring s_para_w(const string& s) {
    if (s.empty()) return wstring();
    int sz = MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), NULL, 0);
    wstring w(sz, 0); MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), &w[0], sz); return w;
}

// --- Estruturas de Dados ---
struct LivroItem { wstring id, tit, aut, tp, gen, prc, est, mari; };
struct ClienteItem { wstring id, nome, cidade, time, anime; };
struct VendedorItem { wstring id, nome; };
struct RankingItem { wstring nome, qtd, total; };
struct HistoricoItem { wstring id_venda, livro, qtd, total, metodo; };

// Variável Global para o Carrinho de Compras
vector<pair<int, int>> carrinhoAtual;

// =========================================================================
//                           BACKEND: LIVROS
// =========================================================================
class LivrariaGerenciador {
private: MYSQL* conexao;
public:
    LivrariaGerenciador() { conexao = mysql_init(NULL); mysql_real_connect(conexao, "localhost", "root", "@GoaT123", "livraria_bd", 3306, NULL, 0); }
    ~LivrariaGerenciador() { if (conexao) mysql_close(conexao); }
    
    bool inserir(wstring t, wstring a, double p, int e, wstring tp, wstring g, bool mari, wstring& erroMsg) { string query = "INSERT INTO livros (titulo, autor, preco, estoque, tipo, genero, produzido_mari) VALUES ('" + w_para_s(t) + "', '" + w_para_s(a) + "', " + to_string(p) + ", " + to_string(e) + ", '" + w_para_s(tp) + "', '" + w_para_s(g) + "', " + (mari ? "1" : "0") + ")"; if (mysql_query(conexao, query.c_str()) == 0) return true; erroMsg = s_para_w(mysql_error(conexao)); return false; }
    bool atualizar(int id, wstring t, wstring a, double p, wstring tp, wstring g, bool mari, wstring& erroMsg) { string query = "UPDATE livros SET titulo = '" + w_para_s(t) + "', autor = '" + w_para_s(a) + "', preco = " + to_string(p) + ", tipo = '" + w_para_s(tp) + "', genero = '" + w_para_s(g) + "', produzido_mari = " + (mari ? "1" : "0") + " WHERE id = " + to_string(id); if (mysql_query(conexao, query.c_str()) == 0) return mysql_affected_rows(conexao) > 0; erroMsg = s_para_w(mysql_error(conexao)); return false; }
    bool buscarPorId(int id, wstring& t, wstring& a, wstring& p, wstring& tp, wstring& g, bool& mari) { string q = "SELECT titulo, autor, preco, tipo, genero, produzido_mari FROM livros WHERE id = " + to_string(id); if (mysql_query(conexao, q.c_str()) == 0) { MYSQL_RES* r = mysql_store_result(conexao); if (r) { MYSQL_ROW row = mysql_fetch_row(r); if (row) { t = row[0]?s_para_w(row[0]):L""; a = row[1]?s_para_w(row[1]):L""; p = row[2]?s_para_w(row[2]):L""; tp = row[3]?s_para_w(row[3]):L""; g = row[4]?s_para_w(row[4]):L""; mari = (row[5] && atoi(row[5]) == 1); mysql_free_result(r); return true; } mysql_free_result(r); } } return false; }
    
    vector<LivroItem> listar(wstring fTit, wstring fAut, wstring fTp, wstring fGen, wstring fPMin, wstring fPMax, bool fMari, bool fEst) { 
        vector<LivroItem> res; 
        string q = "SELECT id, titulo, autor, tipo, genero, preco, estoque, produzido_mari FROM livros WHERE 1=1"; 
        
        if (!fTit.empty()) q += " AND titulo LIKE '%" + w_para_s(fTit) + "%'"; 
        if (!fAut.empty()) q += " AND autor LIKE '%" + w_para_s(fAut) + "%'"; 
        if (fTp != L"Todos") q += " AND tipo = '" + w_para_s(fTp) + "'"; 
        if (fGen != L"Todos") q += " AND genero = '" + w_para_s(fGen) + "'"; 
        if (!fPMin.empty()) q += " AND preco >= " + w_para_s(fPMin);
        if (!fPMax.empty()) q += " AND preco <= " + w_para_s(fPMax);
        if (fMari) q += " AND produzido_mari = 1"; 
        if (fEst) q += " AND estoque < 5"; 

        if (!mysql_query(conexao, q.c_str())) { 
            MYSQL_RES* r = mysql_store_result(conexao); 
            if (r) { 
                MYSQL_ROW l; 
                while ((l = mysql_fetch_row(r))) { 
                    wstring eMari = (l[7] && atoi(l[7]) == 1) ? L"Sim" : L"Não"; 
                    res.push_back({l[0]?s_para_w(l[0]):L"?", l[1]?s_para_w(l[1]):L"", l[2]?s_para_w(l[2]):L"", l[3]?s_para_w(l[3]):L"", l[4]?s_para_w(l[4]):L"", l[5]?s_para_w(l[5]):L"0.00", l[6]?s_para_w(l[6]):L"0", eMari}); 
                } 
                mysql_free_result(r); 
            } 
        } 
        return res; 
    }
    
    int remover(int id) { string q = "DELETE FROM livros WHERE id = " + to_string(id); if (mysql_query(conexao, q.c_str()) == 0) { if (mysql_affected_rows(conexao) > 0) return 1; return 0; } else { if (mysql_errno(conexao) == 1451) return -1; return 0; } }
    bool adicionarEstoque(int id, int qtd) { if (qtd <= 0) return false; string q = "UPDATE livros SET estoque = estoque + " + to_string(qtd) + " WHERE id = " + to_string(id); return mysql_query(conexao, q.c_str()) == 0 && mysql_affected_rows(conexao) > 0; }
    wstring obterRelatorio() { if (!mysql_query(conexao, "SELECT COUNT(*), SUM(estoque), SUM(preco * estoque) FROM livros")) { MYSQL_RES* r = mysql_store_result(conexao); if (r) { MYSQL_ROW l = mysql_fetch_row(r); wstring res = L"Títulos: " + s_para_w(l[0]?l[0]:"0") + L"\nExemplares: " + s_para_w(l[1]?l[1]:"0") + L"\nValor em Estoque: R$ " + s_para_w(l[2]?l[2]:"0.00"); mysql_free_result(r); return res; } } return L"Erro."; }
};

// =========================================================================
//                           BACKEND: CLIENTES & VENDAS
// =========================================================================
class ClienteGerenciador {
private: MYSQL* conexao;
public:
    ClienteGerenciador() { conexao = mysql_init(NULL); mysql_real_connect(conexao, "localhost", "root", "@GoaT123", "livraria_bd", 3306, NULL, 0); }
    ~ClienteGerenciador() { if (conexao) mysql_close(conexao); }
    bool inserir(wstring nome, wstring cidade, wstring time, bool anime, wstring& erroMsg) { string query = "INSERT INTO clientes (nome, cidade, time_coracao, assiste_one_piece) VALUES ('" + w_para_s(nome) + "', '" + w_para_s(cidade) + "', '" + w_para_s(time) + "', " + (anime ? "1" : "0") + ")"; if (mysql_query(conexao, query.c_str()) == 0) return true; erroMsg = s_para_w(mysql_error(conexao)); return false; }
    bool atualizar(int id, wstring nome, wstring cidade, wstring time, bool anime, wstring& erroMsg) { string query = "UPDATE clientes SET nome = '" + w_para_s(nome) + "', cidade = '" + w_para_s(cidade) + "', time_coracao = '" + w_para_s(time) + "', assiste_one_piece = " + (anime ? "1" : "0") + " WHERE id = " + to_string(id); if (mysql_query(conexao, query.c_str()) == 0) return mysql_affected_rows(conexao) > 0; erroMsg = s_para_w(mysql_error(conexao)); return false; }
    vector<ClienteItem> listar() { vector<ClienteItem> res; if (!mysql_query(conexao, "SELECT id, nome, cidade, time_coracao, assiste_one_piece FROM clientes")) { MYSQL_RES* r = mysql_store_result(conexao); if (r) { MYSQL_ROW l; while ((l = mysql_fetch_row(r))) { int op_int = l[4] ? atoi(l[4]) : 0; res.push_back({l[0]?s_para_w(l[0]):L"?", l[1]?s_para_w(l[1]):L"", l[2]?s_para_w(l[2]):L"", l[3]?s_para_w(l[3]):L"", (op_int == 1) ? L"Sim" : L"Não"}); } mysql_free_result(r); } } return res; }
    bool buscarPorId(int id, wstring& nome, wstring& cidade, wstring& time, bool& anime) { string q = "SELECT nome, cidade, time_coracao, assiste_one_piece FROM clientes WHERE id = " + to_string(id); if (mysql_query(conexao, q.c_str()) == 0) { MYSQL_RES* r = mysql_store_result(conexao); if (r) { MYSQL_ROW row = mysql_fetch_row(r); if (row) { nome = row[0]?s_para_w(row[0]):L""; cidade = row[1]?s_para_w(row[1]):L""; time = row[2]?s_para_w(row[2]):L""; anime = (row[3] && atoi(row[3]) == 1); mysql_free_result(r); return true; } mysql_free_result(r); } } return false; }
    vector<HistoricoItem> buscarHistorico(int idCliente, HWND hwnd) { vector<HistoricoItem> res; string q = "SELECT v.id, l.titulo, iv.quantidade, v.valor_total, v.forma_pagamento FROM vendas v INNER JOIN itens_venda iv ON v.id = iv.id_venda INNER JOIN livros l ON iv.id_livro = l.id WHERE v.id_cliente = " + to_string(idCliente) + " ORDER BY v.id DESC"; if (mysql_query(conexao, q.c_str()) != 0) { wstring erro = s_para_w(mysql_error(conexao)); MessageBoxW(hwnd, (L"ERRO NA BUSCA DO HISTÓRICO!\n\nDetalhes do Banco: " + erro).c_str(), L"Aviso de SQL", MB_OK | MB_ICONERROR); return res; } MYSQL_RES* r = mysql_store_result(conexao); if (r) { MYSQL_ROW l; while ((l = mysql_fetch_row(r))) { res.push_back({ l[0]?s_para_w(l[0]):L"?", l[1]?s_para_w(l[1]):L"Livro Apagado", l[2]?s_para_w(l[2]):L"0", l[3]?s_para_w(l[3]):L"0.00", l[4]?s_para_w(l[4]):L"---" }); } mysql_free_result(r); } return res; }
};

class VendasGerenciador {
private: MYSQL* conexao;
public:
    VendasGerenciador() { conexao = mysql_init(NULL); mysql_real_connect(conexao, "localhost", "root", "@GoaT123", "livraria_bd", 3306, NULL, 0); }
    ~VendasGerenciador() { if (conexao) mysql_close(conexao); }
    bool inserirVendedor(wstring nome) { string query = "INSERT INTO vendedores (nome) VALUES ('" + w_para_s(nome) + "')"; return mysql_query(conexao, query.c_str()) == 0; }
    bool atualizarVendedor(int id, wstring nome, wstring& erroMsg) { string query = "UPDATE vendedores SET nome = '" + w_para_s(nome) + "' WHERE id = " + to_string(id); if (mysql_query(conexao, query.c_str()) == 0) return mysql_affected_rows(conexao) > 0; erroMsg = s_para_w(mysql_error(conexao)); return false; }
    vector<VendedorItem> listarVendedores() { vector<VendedorItem> res; if (!mysql_query(conexao, "SELECT id, nome FROM vendedores")) { MYSQL_RES* r = mysql_store_result(conexao); if (r) { MYSQL_ROW l; while ((l = mysql_fetch_row(r))) { res.push_back({l[0]?s_para_w(l[0]):L"?", l[1]?s_para_w(l[1]):L""}); } mysql_free_result(r); } } return res; }
    
    double calcularVenda(int idCli, int idLivro, int qtd, double& precoBase, int& estoqueAtual, wstring& detalhesDesconto, double& totalPerc) { double desconto = 0.0; detalhesDesconto = L""; string qLivro = "SELECT preco, estoque FROM livros WHERE id = " + to_string(idLivro); mysql_query(conexao, qLivro.c_str()); MYSQL_RES* resL = mysql_store_result(conexao); MYSQL_ROW rowL = mysql_fetch_row(resL); if(rowL) { precoBase = atof(rowL[0]); estoqueAtual = atoi(rowL[1]); } else { precoBase = 0.0; estoqueAtual = 0; } mysql_free_result(resL); string qCli = "SELECT cidade, time_coracao, assiste_one_piece FROM clientes WHERE id = " + to_string(idCli); mysql_query(conexao, qCli.c_str()); MYSQL_RES* resC = mysql_store_result(conexao); MYSQL_ROW rowC = mysql_fetch_row(resC); if(rowC) { if(string(rowC[0]) == "Sousa") { desconto += 0.10; detalhesDesconto += L" > Cliente de Sousa: 10%\n"; } if(string(rowC[1]) == "Flamengo") { desconto += 0.05; detalhesDesconto += L" > Torcedor do Flamengo: 5%\n"; } if(atoi(rowC[2]) == 1) { desconto += 0.10; detalhesDesconto += L" > Fã de One Piece: 10%\n"; } } mysql_free_result(resC); if (desconto == 0.0) detalhesDesconto = L" > Nenhum desconto aplicável.\n"; totalPerc = desconto * 100.0; return (precoBase * qtd) * (1.0 - desconto); }
    
    // NOVA FUNCAO COM CARRINHO
    bool finalizarVendaCarrinho(int idCli, int idVend, double total, string met, const vector<pair<int, int>>& carrinho, wstring& erroMsg) {
        if (carrinho.empty()) { erroMsg = L"O carrinho está vazio!"; return false; }

        mysql_query(conexao, "START TRANSACTION");

        string qVenda = "INSERT INTO vendas (id_cliente, id_vendedor, valor_total, forma_pagamento, data_venda) VALUES (" + to_string(idCli) + ", " + to_string(idVend) + ", " + to_string(total) + ", '" + met + "', NOW())";
        if (mysql_query(conexao, qVenda.c_str()) != 0) { erroMsg = s_para_w(mysql_error(conexao)); mysql_query(conexao, "ROLLBACK"); return false; }

        int idVendaGerado = mysql_insert_id(conexao);

        for (const auto& item : carrinho) {
            int idLivro = item.first;
            int qtd = item.second;

            string qItem = "INSERT INTO itens_venda (id_venda, id_livro, quantidade) VALUES (" + to_string(idVendaGerado) + ", " + to_string(idLivro) + ", " + to_string(qtd) + ")";
            if (mysql_query(conexao, qItem.c_str()) != 0) { erroMsg = s_para_w(mysql_error(conexao)); mysql_query(conexao, "ROLLBACK"); return false; }

            string qEstoque = "UPDATE livros SET estoque = estoque - " + to_string(qtd) + " WHERE id = " + to_string(idLivro);
            if (mysql_query(conexao, qEstoque.c_str()) != 0) { erroMsg = s_para_w(mysql_error(conexao)); mysql_query(conexao, "ROLLBACK"); return false; }
        }

        mysql_query(conexao, "COMMIT"); 
        return true;
    }

    vector<RankingItem> listarRanking() { vector<RankingItem> res; if (!mysql_query(conexao, "SELECT * FROM view_relatorio_vendas ORDER BY Faturamento_Total DESC")) { MYSQL_RES* r = mysql_store_result(conexao); if (r) { MYSQL_ROW l; while ((l = mysql_fetch_row(r))) { res.push_back({ l[0]?s_para_w(l[0]):L"?", l[1]?s_para_w(l[1]):L"0", l[2]?s_para_w(l[2]):L"0.00" }); } mysql_free_result(r); } } return res; }
};

LivrariaGerenciador db_livros; ClienteGerenciador db_clientes; VendasGerenciador db_vendas; HFONT hFont;

// =========================================================================
//                           INTERFACE GRÁFICA
// =========================================================================
HWND hMainWindow, hTab; vector<HWND> tabLivros, tabClientes, tabVendedores, tabVendas;

#define ID_BTN_INSERIR     1001
#define ID_BTN_REMOVER     1002
#define ID_BTN_PESQUISAR   1003
#define ID_BTN_RELATORIO   1004
#define ID_BTN_ADD_ESTOQUE 1005
#define ID_BTN_CARREGAR    1006
#define ID_BTN_ATUALIZAR   1007
#define ID_BTN_CLI_INSERIR 2001
#define ID_BTN_CLI_BUSCAR  2002 
#define ID_BTN_CLI_ATUALIZAR 2003
#define ID_BTN_VEND_INSERIR 3001
#define ID_BTN_VEND_ATUALIZAR 3002
#define ID_BTN_FINALIZAR_VENDA 4001
#define ID_BTN_ADD_CARRINHO 4002
#define ID_BTN_REMOVER_CARRINHO 4003
#define ID_EDIT_BUSCA_TITULO 5001 

HWND hBuscaTitulo, hBuscaAutor, hBuscaTipo, hBuscaGenero, hBuscaPrecoMin, hBuscaPrecoMax;
HWND hEditT, hEditA, hEditP, hEditE, hComboTipo, hComboGen, hListViewLivros, hEditIdAcao, hEditQtdAcao, hCheckLivroMari, hFilterMari, hFilterBaixoEstoque; 
HWND hEditCliNome, hEditCliCidade, hEditCliTime, hCheckCliAnime, hListViewClientes, hEditCliBuscaId, hListViewHistorico;
HWND hEditNomeVendedor, hEditVendId, hListViewVendedores;
HWND hEditVenIdCli, hEditVenIdVend, hEditVenIdLivro, hEditVenQtd, hComboMetodo, hListViewRanking, hListCarrinho;

HWND CriarCtrl(vector<HWND>& listaAba, LPCWSTR cls, LPCWSTR txt, DWORD style, int x, int y, int w, int h, HMENU id, bool visivel = false) { DWORD vis = visivel ? WS_VISIBLE : 0; HWND c = CreateWindowExW(0, cls, txt, style | WS_CHILD | vis, x, y, w, h, hMainWindow, id, NULL, NULL); SendMessage(c, WM_SETFONT, (WPARAM)hFont, TRUE); listaAba.push_back(c); return c; }
HWND CriarEdit(vector<HWND>& listaAba, LPCWSTR txt, DWORD style, int x, int y, int w, int h, HMENU id, bool visivel = false) { DWORD vis = visivel ? WS_VISIBLE : 0; HWND c = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", txt, style | WS_CHILD | vis, x, y, w, h, hMainWindow, id, NULL, NULL); SendMessage(c, WM_SETFONT, (WPARAM)hFont, TRUE); listaAba.push_back(c); return c; }

void ConfigListView(HWND hList, const vector<wstring>& cols, const vector<int>& widths) {
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    LVCOLUMNW lvc = {0}; lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    for (size_t i = 0; i < cols.size(); i++) { lvc.iSubItem = i; lvc.cx = widths[i]; lvc.pszText = (LPWSTR)cols[i].c_str(); ListView_InsertColumn(hList, i, &lvc); }
}

void AtualizarTabelaLivros() {
    ListView_DeleteAllItems(hListViewLivros); 
    
    wchar_t t[256], a[256], tp[50], g[50], pMin[20], pMax[20];
    GetWindowTextW(hBuscaTitulo, t, 256); GetWindowTextW(hBuscaAutor, a, 256); GetWindowTextW(hBuscaPrecoMin, pMin, 20); GetWindowTextW(hBuscaPrecoMax, pMax, 20);
    SendMessage(hBuscaTipo, CB_GETLBTEXT, SendMessage(hBuscaTipo, CB_GETCURSEL, 0, 0), (LPARAM)tp); SendMessage(hBuscaGenero, CB_GETLBTEXT, SendMessage(hBuscaGenero, CB_GETCURSEL, 0, 0), (LPARAM)g);
    bool fMari = (SendMessage(hFilterMari, BM_GETCHECK, 0, 0) == BST_CHECKED); bool fEst = (SendMessage(hFilterBaixoEstoque, BM_GETCHECK, 0, 0) == BST_CHECKED);

    vector<LivroItem> l = db_livros.listar(t, a, tp, g, pMin, pMax, fMari, fEst);
    for(size_t i=0; i<l.size(); i++){ 
        LVITEMW lvi={0}; lvi.mask=LVIF_TEXT; lvi.iItem=i; lvi.iSubItem=0; lvi.pszText=(LPWSTR)l[i].id.c_str(); 
        ListView_InsertItem(hListViewLivros, &lvi); 
        ListView_SetItemText(hListViewLivros, i, 1, (LPWSTR)l[i].tit.c_str()); ListView_SetItemText(hListViewLivros, i, 2, (LPWSTR)l[i].aut.c_str()); 
        ListView_SetItemText(hListViewLivros, i, 3, (LPWSTR)l[i].tp.c_str()); ListView_SetItemText(hListViewLivros, i, 4, (LPWSTR)l[i].gen.c_str()); 
        ListView_SetItemText(hListViewLivros, i, 5, (LPWSTR)l[i].prc.c_str()); ListView_SetItemText(hListViewLivros, i, 6, (LPWSTR)l[i].est.c_str()); 
        ListView_SetItemText(hListViewLivros, i, 7, (LPWSTR)l[i].mari.c_str());
    }
}

void AtualizarTabelasGlobais() {
    AtualizarTabelaLivros();
    ListView_DeleteAllItems(hListViewClientes); vector<ClienteItem> c = db_clientes.listar(); for(size_t i=0; i<c.size(); i++){ LVITEMW lvi={0}; lvi.mask=LVIF_TEXT; lvi.iItem=i; lvi.iSubItem=0; lvi.pszText=(LPWSTR)c[i].id.c_str(); ListView_InsertItem(hListViewClientes, &lvi); ListView_SetItemText(hListViewClientes, i, 1, (LPWSTR)c[i].nome.c_str()); ListView_SetItemText(hListViewClientes, i, 2, (LPWSTR)c[i].cidade.c_str()); ListView_SetItemText(hListViewClientes, i, 3, (LPWSTR)c[i].time.c_str()); ListView_SetItemText(hListViewClientes, i, 4, (LPWSTR)c[i].anime.c_str()); }
    ListView_DeleteAllItems(hListViewVendedores); vector<VendedorItem> v = db_vendas.listarVendedores(); for(size_t i=0; i<v.size(); i++){ LVITEMW lvi={0}; lvi.mask=LVIF_TEXT; lvi.iItem=i; lvi.iSubItem=0; lvi.pszText=(LPWSTR)v[i].id.c_str(); ListView_InsertItem(hListViewVendedores, &lvi); ListView_SetItemText(hListViewVendedores, i, 1, (LPWSTR)v[i].nome.c_str()); }
    ListView_DeleteAllItems(hListViewRanking); vector<RankingItem> r = db_vendas.listarRanking(); for(size_t i=0; i<r.size(); i++){ LVITEMW lvi={0}; lvi.mask=LVIF_TEXT; lvi.iItem=i; lvi.iSubItem=0; lvi.pszText=(LPWSTR)r[i].nome.c_str(); ListView_InsertItem(hListViewRanking, &lvi); ListView_SetItemText(hListViewRanking, i, 1, (LPWSTR)r[i].qtd.c_str()); ListView_SetItemText(hListViewRanking, i, 2, (LPWSTR)(L"R$ " + r[i].total).c_str()); }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            hMainWindow = hwnd; hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            hTab = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 10, 10, 1000, 480, hwnd, (HMENU)999, NULL, NULL); SendMessage(hTab, WM_SETFONT, (WPARAM)hFont, TRUE);
            TCITEMW tie; tie.mask = TCIF_TEXT; tie.pszText = (LPWSTR)L"📦 Estoque"; TabCtrl_InsertItem(hTab, 0, &tie); tie.pszText = (LPWSTR)L"👥 CRM (Clientes)"; TabCtrl_InsertItem(hTab, 1, &tie); tie.pszText = (LPWSTR)L"👔 Vendedores"; TabCtrl_InsertItem(hTab, 2, &tie); tie.pszText = (LPWSTR)L"🛒 Vendas & BI"; TabCtrl_InsertItem(hTab, 3, &tie);

            // ABA 1: LIVROS 
            CriarCtrl(tabLivros, L"STATIC", L"Título:", 0, 30, 60, 60, 20, NULL, true); hEditT = CriarEdit(tabLivros, L"", 0, 95, 60, 180, 24, NULL, true);
            CriarCtrl(tabLivros, L"STATIC", L"Autor:", 0, 30, 95, 60, 20, NULL, true); hEditA = CriarEdit(tabLivros, L"", 0, 95, 95, 180, 24, NULL, true);
            CriarCtrl(tabLivros, L"STATIC", L"Tipo:", 0, 30, 130, 60, 20, NULL, true); hComboTipo = CriarCtrl(tabLivros, L"COMBOBOX", L"", CBS_DROPDOWNLIST, 95, 130, 180, 150, NULL, true); SendMessage(hComboTipo, CB_ADDSTRING, 0, (LPARAM)L"Livro"); SendMessage(hComboTipo, CB_ADDSTRING, 0, (LPARAM)L"Mangá"); SendMessage(hComboTipo, CB_SETCURSEL, 0, 0);
            CriarCtrl(tabLivros, L"STATIC", L"Gênero:", 0, 30, 165, 60, 20, NULL, true); hComboGen = CriarCtrl(tabLivros, L"COMBOBOX", L"", CBS_DROPDOWNLIST, 95, 165, 180, 150, NULL, true); const wchar_t* gens[] = { L"Aventura", L"Ação", L"Romance", L"Terror", L"Ficção", L"Comédia" }; for(int i=0; i<6; i++) SendMessage(hComboGen, CB_ADDSTRING, 0, (LPARAM)gens[i]); SendMessage(hComboGen, CB_SETCURSEL, 0, 0);
            CriarCtrl(tabLivros, L"STATIC", L"Preço:", 0, 30, 200, 60, 20, NULL, true); hEditP = CriarEdit(tabLivros, L"", 0, 95, 200, 70, 24, NULL, true);
            CriarCtrl(tabLivros, L"STATIC", L"Qtd:", 0, 175, 200, 40, 20, NULL, true); hEditE = CriarEdit(tabLivros, L"", ES_NUMBER, 220, 200, 55, 24, NULL, true);
            hCheckLivroMari = CriarCtrl(tabLivros, L"BUTTON", L"Produzido em Mari (PB)?", BS_AUTOCHECKBOX, 30, 235, 200, 20, NULL, true);
            CriarCtrl(tabLivros, L"BUTTON", L"Cadastrar Novo Título", BS_PUSHBUTTON, 30, 265, 265, 30, (HMENU)ID_BTN_INSERIR, true);
            CriarCtrl(tabLivros, L"BUTTON", L"Salvar Alterações (Edição)", BS_PUSHBUTTON, 30, 300, 265, 30, (HMENU)ID_BTN_ATUALIZAR, true);
            CriarCtrl(tabLivros, L"BUTTON", L"Gerar Relatório de Patrimônio", BS_PUSHBUTTON, 30, 335, 265, 30, (HMENU)ID_BTN_RELATORIO, true);
            
            CriarCtrl(tabLivros, L"STATIC", L"🔎 Filtros de Busca", 0, 340, 45, 200, 20, NULL, true); 
            CriarCtrl(tabLivros, L"STATIC", L"Título:", 0, 340, 75, 50, 20, NULL, true); hBuscaTitulo = CriarEdit(tabLivros, L"", 0, 395, 72, 140, 24, (HMENU)ID_EDIT_BUSCA_TITULO, true);
            CriarCtrl(tabLivros, L"STATIC", L"Autor:", 0, 545, 75, 50, 20, NULL, true); hBuscaAutor = CriarEdit(tabLivros, L"", 0, 595, 72, 140, 24, NULL, true);
            CriarCtrl(tabLivros, L"BUTTON", L"Buscar / Filtrar", BS_PUSHBUTTON, 750, 71, 120, 60, (HMENU)ID_BTN_PESQUISAR, true);
            CriarCtrl(tabLivros, L"STATIC", L"Tipo:", 0, 340, 108, 40, 20, NULL, true); hBuscaTipo = CriarCtrl(tabLivros, L"COMBOBOX", L"", CBS_DROPDOWNLIST, 385, 105, 100, 150, NULL, true); SendMessage(hBuscaTipo, CB_ADDSTRING, 0, (LPARAM)L"Todos"); SendMessage(hBuscaTipo, CB_ADDSTRING, 0, (LPARAM)L"Livro"); SendMessage(hBuscaTipo, CB_ADDSTRING, 0, (LPARAM)L"Mangá"); SendMessage(hBuscaTipo, CB_SETCURSEL, 0, 0); 
            CriarCtrl(tabLivros, L"STATIC", L"Gênero:", 0, 495, 108, 60, 20, NULL, true); hBuscaGenero = CriarCtrl(tabLivros, L"COMBOBOX", L"", CBS_DROPDOWNLIST, 560, 105, 120, 150, NULL, true); const wchar_t* gensBusca[] = { L"Todos", L"Aventura", L"Ação", L"Romance", L"Terror", L"Ficção", L"Comédia" }; for(int i=0; i<7; i++) SendMessage(hBuscaGenero, CB_ADDSTRING, 0, (LPARAM)gensBusca[i]); SendMessage(hBuscaGenero, CB_SETCURSEL, 0, 0); 
            CriarCtrl(tabLivros, L"STATIC", L"R$ Min:", 0, 340, 143, 50, 20, NULL, true); hBuscaPrecoMin = CriarEdit(tabLivros, L"", 0, 395, 140, 60, 24, NULL, true);
            CriarCtrl(tabLivros, L"STATIC", L"R$ Max:", 0, 465, 143, 50, 20, NULL, true); hBuscaPrecoMax = CriarEdit(tabLivros, L"", 0, 520, 140, 60, 24, NULL, true);
            hFilterMari = CriarCtrl(tabLivros, L"BUTTON", L"Apenas Mari", BS_AUTOCHECKBOX, 595, 142, 110, 20, NULL, true); 
            hFilterBaixoEstoque = CriarCtrl(tabLivros, L"BUTTON", L"Estoque Baixo", BS_AUTOCHECKBOX, 715, 142, 160, 20, NULL, true);

            CriarCtrl(tabLivros, L"STATIC", L"ID:", 0, 340, 172, 30, 20, NULL, true); hEditIdAcao = CriarEdit(tabLivros, L"", ES_NUMBER, 370, 170, 40, 24, NULL, true);
            CriarCtrl(tabLivros, L"BUTTON", L"Carregar", BS_PUSHBUTTON, 420, 169, 80, 26, (HMENU)ID_BTN_CARREGAR, true); CriarCtrl(tabLivros, L"BUTTON", L"Remover", BS_PUSHBUTTON, 510, 169, 80, 26, (HMENU)ID_BTN_REMOVER, true);
            CriarCtrl(tabLivros, L"STATIC", L"+ Qtd:", 0, 610, 172, 45, 20, NULL, true); hEditQtdAcao = CriarEdit(tabLivros, L"", ES_NUMBER, 660, 170, 50, 24, NULL, true); CriarCtrl(tabLivros, L"BUTTON", L"Add", BS_PUSHBUTTON, 720, 169, 60, 26, (HMENU)ID_BTN_ADD_ESTOQUE, true);
            
            hListViewLivros = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL, 340, 205, 650, 290, hwnd, NULL, NULL, NULL); 
            SendMessage(hListViewLivros, WM_SETFONT, (WPARAM)hFont, TRUE); tabLivros.push_back(hListViewLivros);
            ConfigListView(hListViewLivros, {L"ID", L"Título", L"Autor", L"Tipo", L"Gênero", L"Preço", L"Qtd", L"Mari?"}, {40, 150, 120, 70, 80, 70, 40, 60});

            // ABA 2: CLIENTES
            CriarCtrl(tabClientes, L"STATIC", L"Nome do Cliente:", 0, 30, 60, 150, 20, NULL); hEditCliNome = CriarEdit(tabClientes, L"", 0, 30, 85, 250, 24, NULL); 
            CriarCtrl(tabClientes, L"STATIC", L"Cidade:", 0, 30, 120, 150, 20, NULL); hEditCliCidade = CriarEdit(tabClientes, L"", 0, 30, 145, 250, 24, NULL); 
            CriarCtrl(tabClientes, L"STATIC", L"Time:", 0, 30, 180, 150, 20, NULL); hEditCliTime = CriarEdit(tabClientes, L"", 0, 30, 205, 250, 24, NULL); 
            hCheckCliAnime = CriarCtrl(tabClientes, L"BUTTON", L"Assiste One Piece? (Desconto)", BS_AUTOCHECKBOX, 30, 240, 250, 20, NULL); 
            CriarCtrl(tabClientes, L"BUTTON", L"Cadastrar Novo Cliente", BS_PUSHBUTTON, 30, 280, 250, 35, (HMENU)ID_BTN_CLI_INSERIR); 
            CriarCtrl(tabClientes, L"BUTTON", L"Salvar Alterações (Edição)", BS_PUSHBUTTON, 30, 320, 250, 35, (HMENU)ID_BTN_CLI_ATUALIZAR);
            
            hListViewClientes = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 320, 60, 670, 180, hwnd, NULL, NULL, NULL); SendMessage(hListViewClientes, WM_SETFONT, (WPARAM)hFont, TRUE); tabClientes.push_back(hListViewClientes); ConfigListView(hListViewClientes, {L"ID", L"Nome do Cliente", L"Cidade", L"Time", L"Fã O.P.?"}, {40, 200, 120, 120, 100}); 
            CriarCtrl(tabClientes, L"STATIC", L"ID do Cliente:", 0, 320, 255, 100, 20, NULL); hEditCliBuscaId = CriarEdit(tabClientes, L"", ES_NUMBER, 430, 252, 60, 24, NULL); 
            CriarCtrl(tabClientes, L"BUTTON", L"Ver Dados Cadastrais & Histórico", BS_PUSHBUTTON, 500, 250, 250, 28, (HMENU)ID_BTN_CLI_BUSCAR); 
            CriarCtrl(tabClientes, L"STATIC", L"Histórico de Compras Realizadas:", 0, 320, 290, 300, 20, NULL); hListViewHistorico = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 320, 315, 670, 145, hwnd, NULL, NULL, NULL); SendMessage(hListViewHistorico, WM_SETFONT, (WPARAM)hFont, TRUE); tabClientes.push_back(hListViewHistorico); ConfigListView(hListViewHistorico, {L"Venda ID", L"Livro Comprado", L"Qtd", L"Valor Pago (R$)", L"Forma Pgto"}, {80, 250, 50, 120, 120});

            // ABA 3: VENDEDORES
            CriarCtrl(tabVendedores, L"STATIC", L"Nome do Vendedor:", 0, 30, 60, 150, 20, NULL); hEditNomeVendedor = CriarEdit(tabVendedores, L"", 0, 30, 85, 250, 24, NULL); 
            CriarCtrl(tabVendedores, L"BUTTON", L"Registrar Vendedor", BS_PUSHBUTTON, 30, 120, 250, 35, (HMENU)ID_BTN_VEND_INSERIR); 
            CriarCtrl(tabVendedores, L"STATIC", L"ID (Para Editar):", 0, 30, 165, 120, 20, NULL); hEditVendId = CriarEdit(tabVendedores, L"", ES_NUMBER, 30, 190, 60, 24, NULL);
            CriarCtrl(tabVendedores, L"BUTTON", L"Atualizar Vendedor", BS_PUSHBUTTON, 100, 185, 180, 35, (HMENU)ID_BTN_VEND_ATUALIZAR);
            hListViewVendedores = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 320, 60, 670, 400, hwnd, NULL, NULL, NULL); SendMessage(hListViewVendedores, WM_SETFONT, (WPARAM)hFont, TRUE); tabVendedores.push_back(hListViewVendedores); ConfigListView(hListViewVendedores, {L"ID", L"Nome do Vendedor"}, {50, 300});

            // ABA 4: VENDAS E CARRINHO
            CriarCtrl(tabVendas, L"STATIC", L"ID Cliente:", 0, 30, 60, 120, 20, NULL); hEditVenIdCli = CriarEdit(tabVendas, L"", ES_NUMBER, 150, 60, 60, 24, NULL); 
            CriarCtrl(tabVendas, L"STATIC", L"ID Vendedor:", 0, 30, 95, 120, 20, NULL); hEditVenIdVend = CriarEdit(tabVendas, L"", ES_NUMBER, 150, 95, 60, 24, NULL); 
            CriarCtrl(tabVendas, L"STATIC", L"Pagamento:", 0, 30, 130, 120, 20, NULL); hComboMetodo = CriarCtrl(tabVendas, L"COMBOBOX", L"", CBS_DROPDOWNLIST, 150, 130, 120, 100, NULL); SendMessage(hComboMetodo, CB_ADDSTRING, 0, (LPARAM)L"Pix"); SendMessage(hComboMetodo, CB_ADDSTRING, 0, (LPARAM)L"Cartão"); SendMessage(hComboMetodo, CB_ADDSTRING, 0, (LPARAM)L"Berries"); SendMessage(hComboMetodo, CB_SETCURSEL, 0, 0); 
            
            CriarCtrl(tabVendas, L"STATIC", L"ID Livro:", 0, 30, 175, 120, 20, NULL); hEditVenIdLivro = CriarEdit(tabVendas, L"", ES_NUMBER, 150, 175, 60, 24, NULL); 
            CriarCtrl(tabVendas, L"STATIC", L"Quantidade:", 0, 30, 210, 120, 20, NULL); hEditVenQtd = CriarEdit(tabVendas, L"1", ES_NUMBER, 150, 210, 60, 24, NULL); 
            
            CriarCtrl(tabVendas, L"BUTTON", L"Adicionar ao Carrinho", BS_PUSHBUTTON, 30, 245, 150, 30, (HMENU)ID_BTN_ADD_CARRINHO);
            CriarCtrl(tabVendas, L"BUTTON", L"Remover Item", BS_PUSHBUTTON, 190, 245, 110, 30, (HMENU)ID_BTN_REMOVER_CARRINHO);
            CriarCtrl(tabVendas, L"STATIC", L"🛒 Itens no Carrinho:", 0, 30, 285, 250, 20, NULL);
            hListCarrinho = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTBOXW, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, 30, 305, 250, 100, hwnd, NULL, NULL, NULL); SendMessage(hListCarrinho, WM_SETFONT, (WPARAM)hFont, TRUE); tabVendas.push_back(hListCarrinho);
            CriarCtrl(tabVendas, L"BUTTON", L"Finalizar Venda do Carrinho", BS_PUSHBUTTON, 30, 415, 250, 35, (HMENU)ID_BTN_FINALIZAR_VENDA);
            
            CriarCtrl(tabVendas, L"STATIC", L"🏆 Relatório BI: Ranking de Desempenho (View)", 0, 320, 60, 400, 20, NULL); hListViewRanking = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 320, 85, 670, 375, hwnd, NULL, NULL, NULL); SendMessage(hListViewRanking, WM_SETFONT, (WPARAM)hFont, TRUE); tabVendas.push_back(hListViewRanking); ConfigListView(hListViewRanking, {L"Vendedor", L"Qtd Vendas", L"Faturamento Total"}, {250, 120, 200});

            AtualizarTabelasGlobais();
            return 0;
        }

        case WM_NOTIFY: {
            if (((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
                int aba = TabCtrl_GetCurSel(hTab);
                for(HWND h : tabLivros) ShowWindow(h, (aba == 0) ? SW_SHOW : SW_HIDE);
                for(HWND h : tabClientes) ShowWindow(h, (aba == 1) ? SW_SHOW : SW_HIDE);
                for(HWND h : tabVendedores) ShowWindow(h, (aba == 2) ? SW_SHOW : SW_HIDE);
                for(HWND h : tabVendas) ShowWindow(h, (aba == 3) ? SW_SHOW : SW_HIDE);
            }
            return 0;
        }

        case WM_SIZE: {
            int w = LOWORD(lParam); int h = HIWORD(lParam);
            SetWindowPos(hTab, NULL, 10, 10, w - 20, h - 20, SWP_NOZORDER);
            if (hListViewLivros) SetWindowPos(hListViewLivros, NULL, 340, 205, w - 370, h - 235, SWP_NOZORDER);
            if (hListViewClientes) SetWindowPos(hListViewClientes, NULL, 320, 60, w - 350, 180, SWP_NOZORDER);
            if (hListViewHistorico) SetWindowPos(hListViewHistorico, NULL, 320, 315, w - 350, h - 355, SWP_NOZORDER);
            if (hListViewVendedores) SetWindowPos(hListViewVendedores, NULL, 320, 60, w - 350, h - 90, SWP_NOZORDER);
            if (hListViewRanking) SetWindowPos(hListViewRanking, NULL, 320, 85, w - 350, h - 115, SWP_NOZORDER);
            return 0;
        }

        case WM_COMMAND: {
            if (HIWORD(wParam) == EN_CHANGE) { if (LOWORD(wParam) == ID_EDIT_BUSCA_TITULO) AtualizarTabelaLivros(); }

            // Lógica de Livros
            if (LOWORD(wParam) == ID_BTN_INSERIR) { wchar_t t[256], a[256], p[20], e[20], tp[50], g[50]; GetWindowTextW(hEditT, t, 256); GetWindowTextW(hEditA, a, 256); GetWindowTextW(hEditP, p, 20); GetWindowTextW(hEditE, e, 20); if (wcslen(t) == 0 || wcslen(a) == 0 || wcslen(p) == 0 || wcslen(e) == 0) { MessageBoxW(hwnd, L"Preencha todos os campos!", L"Aviso", MB_OK); return 0; } SendMessage(hComboTipo, CB_GETLBTEXT, SendMessage(hComboTipo, CB_GETCURSEL, 0, 0), (LPARAM)tp); SendMessage(hComboGen, CB_GETLBTEXT, SendMessage(hComboGen, CB_GETCURSEL, 0, 0), (LPARAM)g); bool mari = (SendMessage(hCheckLivroMari, BM_GETCHECK, 0, 0) == BST_CHECKED); wstring msgErro; if (db_livros.inserir(t, a, _wtof(p), _wtoi(e), tp, g, mari, msgErro)) { MessageBoxW(hwnd, L"Cadastrado com Sucesso!", L"Sucesso", MB_OK); AtualizarTabelasGlobais(); SetWindowTextW(hEditT, L""); SetWindowTextW(hEditA, L""); SetWindowTextW(hEditP, L""); SetWindowTextW(hEditE, L""); SendMessage(hCheckLivroMari, BM_SETCHECK, BST_UNCHECKED, 0); } else MessageBoxW(hwnd, (L"Erro:\n" + msgErro).c_str(), L"Erro", MB_OK); }
            if (LOWORD(wParam) == ID_BTN_CARREGAR) { wchar_t idB[20]; GetWindowTextW(hEditIdAcao, idB, 20); if (wcslen(idB) > 0) { wstring t, a, p, tp, g; bool mari; if (db_livros.buscarPorId(_wtoi(idB), t, a, p, tp, g, mari)) { SetWindowTextW(hEditT, t.c_str()); SetWindowTextW(hEditA, a.c_str()); SetWindowTextW(hEditP, p.c_str()); SendMessage(hComboTipo, CB_SELECTSTRING, -1, (LPARAM)tp.c_str()); SendMessage(hComboGen, CB_SELECTSTRING, -1, (LPARAM)g.c_str()); SetWindowTextW(hEditE, L""); SendMessage(hCheckLivroMari, BM_SETCHECK, mari ? BST_CHECKED : BST_UNCHECKED, 0); } else MessageBoxW(hwnd, L"ID não encontrado.", L"Erro", MB_OK); } }
            if (LOWORD(wParam) == ID_BTN_ATUALIZAR) { wchar_t idB[20]; GetWindowTextW(hEditIdAcao, idB, 20); if (wcslen(idB) == 0) return 0; wchar_t t[256], a[256], p[20], tp[50], g[50]; GetWindowTextW(hEditT, t, 256); GetWindowTextW(hEditA, a, 256); GetWindowTextW(hEditP, p, 20); SendMessage(hComboTipo, CB_GETLBTEXT, SendMessage(hComboTipo, CB_GETCURSEL, 0, 0), (LPARAM)tp); SendMessage(hComboGen, CB_GETLBTEXT, SendMessage(hComboGen, CB_GETCURSEL, 0, 0), (LPARAM)g); bool mari = (SendMessage(hCheckLivroMari, BM_GETCHECK, 0, 0) == BST_CHECKED); wstring msgErro; if (db_livros.atualizar(_wtoi(idB), t, a, _wtof(p), tp, g, mari, msgErro)) { MessageBoxW(hwnd, L"Atualizado!", L"Sucesso", MB_OK); AtualizarTabelasGlobais(); SetWindowTextW(hEditIdAcao, L""); } }
            if (LOWORD(wParam) == ID_BTN_PESQUISAR) AtualizarTabelaLivros();
            if (LOWORD(wParam) == ID_BTN_ADD_ESTOQUE) { wchar_t idW[20], qtdW[20]; GetWindowTextW(hEditIdAcao, idW, 20); GetWindowTextW(hEditQtdAcao, qtdW, 20); if (wcslen(idW) > 0 && wcslen(qtdW) > 0) { if (db_livros.adicionarEstoque(_wtoi(idW), _wtoi(qtdW))) { MessageBoxW(hwnd, L"Estoque add!", L"Sucesso", MB_OK); SetWindowTextW(hEditIdAcao, L""); SetWindowTextW(hEditQtdAcao, L""); AtualizarTabelasGlobais(); } } }
            if (LOWORD(wParam) == ID_BTN_REMOVER) { wchar_t idB[20]; GetWindowTextW(hEditIdAcao, idB, 20); if (wcslen(idB) > 0) { if (MessageBoxW(hwnd, L"Remover livro?", L"Atenção", MB_YESNO) == IDYES) { int s = db_livros.remover(_wtoi(idB)); if (s == 1) { MessageBoxW(hwnd, L"Removido!", L"Sucesso", MB_OK); SetWindowTextW(hEditIdAcao, L""); AtualizarTabelasGlobais(); } else if (s == -1) MessageBoxW(hwnd, L"BLOQUEADO: Possui vendas.", L"Erro", MB_OK); } } }
            if (LOWORD(wParam) == ID_BTN_RELATORIO) MessageBoxW(hwnd, db_livros.obterRelatorio().c_str(), L"Patrimônio", MB_OK);
            
            // Lógica de Clientes
            if (LOWORD(wParam) == ID_BTN_CLI_INSERIR) { wchar_t n[256], c[100], t[100]; GetWindowTextW(hEditCliNome, n, 256); GetWindowTextW(hEditCliCidade, c, 100); GetWindowTextW(hEditCliTime, t, 100); bool anime = (SendMessage(hCheckCliAnime, BM_GETCHECK, 0, 0) == BST_CHECKED); if (wcslen(n) == 0) return 0; wstring msgErro; if (db_clientes.inserir(n, c, t, anime, msgErro)) { MessageBoxW(hwnd, L"Cliente salvo!", L"Sucesso", MB_OK); AtualizarTabelasGlobais(); SetWindowTextW(hEditCliNome, L""); SetWindowTextW(hEditCliCidade, L""); SetWindowTextW(hEditCliTime, L""); SendMessage(hCheckCliAnime, BM_SETCHECK, BST_UNCHECKED, 0); } }
            if (LOWORD(wParam) == ID_BTN_CLI_ATUALIZAR) { wchar_t idW[20]; GetWindowTextW(hEditCliBuscaId, idW, 20); if (wcslen(idW) == 0) { MessageBoxW(hwnd, L"Primeiro busque o ID!", L"Aviso", MB_OK); return 0; } wchar_t n[256], c[100], t[100]; GetWindowTextW(hEditCliNome, n, 256); GetWindowTextW(hEditCliCidade, c, 100); GetWindowTextW(hEditCliTime, t, 100); bool anime = (SendMessage(hCheckCliAnime, BM_GETCHECK, 0, 0) == BST_CHECKED); if (wcslen(n) == 0) return 0; wstring msgErro; if (db_clientes.atualizar(_wtoi(idW), n, c, t, anime, msgErro)) { MessageBoxW(hwnd, L"Cliente atualizado!", L"Sucesso", MB_OK); AtualizarTabelasGlobais(); } else MessageBoxW(hwnd, (L"Erro:\n" + msgErro).c_str(), L"Erro", MB_OK); }
            if (LOWORD(wParam) == ID_BTN_CLI_BUSCAR) { wchar_t idW[20]; GetWindowTextW(hEditCliBuscaId, idW, 20); if (wcslen(idW) > 0) { int id = _wtoi(idW); wstring n, c, t; bool a; if (db_clientes.buscarPorId(id, n, c, t, a)) { SetWindowTextW(hEditCliNome, n.c_str()); SetWindowTextW(hEditCliCidade, c.c_str()); SetWindowTextW(hEditCliTime, t.c_str()); SendMessage(hCheckCliAnime, BM_SETCHECK, a ? BST_CHECKED : BST_UNCHECKED, 0); ListView_DeleteAllItems(hListViewHistorico); vector<HistoricoItem> hist = db_clientes.buscarHistorico(id, hwnd); if (hist.empty()) MessageBoxW(hwnd, L"Cliente carregado! Mas ainda não fez nenhuma compra.", L"Histórico", MB_OK | MB_ICONINFORMATION); else { for(size_t i=0; i<hist.size(); i++){ LVITEMW lvi={0}; lvi.mask=LVIF_TEXT; lvi.iItem=i; lvi.iSubItem=0; lvi.pszText=(LPWSTR)hist[i].id_venda.c_str(); ListView_InsertItem(hListViewHistorico, &lvi); ListView_SetItemText(hListViewHistorico, i, 1, (LPWSTR)hist[i].livro.c_str()); ListView_SetItemText(hListViewHistorico, i, 2, (LPWSTR)hist[i].qtd.c_str()); ListView_SetItemText(hListViewHistorico, i, 3, (LPWSTR)hist[i].total.c_str()); ListView_SetItemText(hListViewHistorico, i, 4, (LPWSTR)hist[i].metodo.c_str()); } } } else MessageBoxW(hwnd, L"Cliente não encontrado.", L"Erro", MB_OK); } }
            
            // Lógica de Vendedores
            if (LOWORD(wParam) == ID_BTN_VEND_INSERIR) { wchar_t n[256]; GetWindowTextW(hEditNomeVendedor, n, 256); if (wcslen(n) == 0) return 0; if (db_vendas.inserirVendedor(n)) { MessageBoxW(hwnd, L"Vendedor registrado!", L"Sucesso", MB_OK); SetWindowTextW(hEditNomeVendedor, L""); AtualizarTabelasGlobais(); } }
            if (LOWORD(wParam) == ID_BTN_VEND_ATUALIZAR) { wchar_t idW[20], n[256]; GetWindowTextW(hEditVendId, idW, 20); GetWindowTextW(hEditNomeVendedor, n, 256); if (wcslen(idW) == 0 || wcslen(n) == 0) return 0; wstring msgErro; if (db_vendas.atualizarVendedor(_wtoi(idW), n, msgErro)) { MessageBoxW(hwnd, L"Vendedor atualizado!", L"Sucesso", MB_OK); SetWindowTextW(hEditNomeVendedor, L""); SetWindowTextW(hEditVendId, L""); AtualizarTabelasGlobais(); } else MessageBoxW(hwnd, (L"Erro:\n" + msgErro).c_str(), L"Erro", MB_OK); }
            
            // Lógica do Carrinho e Finalização de Vendas
            if (LOWORD(wParam) == ID_BTN_ADD_CARRINHO) {
                wchar_t l[10], q[10]; GetWindowTextW(hEditVenIdLivro, l, 10); GetWindowTextW(hEditVenQtd, q, 10);
                if (wcslen(l) == 0 || wcslen(q) == 0) { MessageBoxW(hwnd, L"Digite ID do Livro e Qtd!", L"Aviso", MB_OK); return 0; }
                int idL = _wtoi(l); int qtdL = _wtoi(q);
                carrinhoAtual.push_back({idL, qtdL});
                wstring exibe = L"ID Livro: " + to_wstring(idL) + L" | Qtd: " + to_wstring(qtdL);
                SendMessage(hListCarrinho, LB_ADDSTRING, 0, (LPARAM)exibe.c_str());
                SetWindowTextW(hEditVenIdLivro, L""); SetWindowTextW(hEditVenQtd, L"1");
            }
            if (LOWORD(wParam) == ID_BTN_REMOVER_CARRINHO) {
                // Pega o índice (linha) do item que está selecionado na caixinha
                int index = SendMessage(hListCarrinho, LB_GETCURSEL, 0, 0);
                
                if (index != LB_ERR) { // LB_ERR significa que nada foi selecionado
                    // Remove o item da nossa variável na memória (Backend)
                    carrinhoAtual.erase(carrinhoAtual.begin() + index);
                    // Remove o item da lista visual (Frontend)
                    SendMessage(hListCarrinho, LB_DELETESTRING, index, 0);
                } else {
                    MessageBoxW(hwnd, L"Clique em um item na lista do carrinho para poder remover!", L"Aviso", MB_OK | MB_ICONWARNING);
                }
            }
            if (LOWORD(wParam) == ID_BTN_FINALIZAR_VENDA) {
                if (carrinhoAtual.empty()) { MessageBoxW(hwnd, L"O carrinho está vazio!", L"Erro", MB_OK); return 0; }
                wchar_t c[10], v[10], m[50]; GetWindowTextW(hEditVenIdCli, c, 10); GetWindowTextW(hEditVenIdVend, v, 10); SendMessage(hComboMetodo, CB_GETLBTEXT, SendMessage(hComboMetodo, CB_GETCURSEL, 0, 0), (LPARAM)m);
                if (wcslen(c) == 0 || wcslen(v) == 0) { MessageBoxW(hwnd, L"Preencha ID Cliente e Vendedor!", L"Erro", MB_OK); return 0; }
                double totalGeral = 0.0; double totalDescontosPercent = 0.0; wstring detalhesGerais = L"";
                
                for (size_t i = 0; i < carrinhoAtual.size(); i++) {
                    double pBase = 0; int estAtual = 0; double percD = 0; wstring detD;
                    double tItem = db_vendas.calcularVenda(_wtoi(c), carrinhoAtual[i].first, carrinhoAtual[i].second, pBase, estAtual, detD, percD);
                    if (pBase == 0) { MessageBoxW(hwnd, (L"Livro ID " + to_wstring(carrinhoAtual[i].first) + L" não existe!").c_str(), L"Erro", MB_OK); return 0; }
                    if (estAtual < carrinhoAtual[i].second) { MessageBoxW(hwnd, (L"Estoque insuficiente para o Livro ID " + to_wstring(carrinhoAtual[i].first)).c_str(), L"Erro", MB_OK); return 0; }
                    totalGeral += tItem;
                    if (i == 0) { totalDescontosPercent = percD; detalhesGerais = detD; }
                }

                wchar_t bufT[50]; swprintf(bufT, 50, L"%.2f", totalGeral);
                wstring msg = L"====== RESUMO DO CARRINHO ======\n\nItens: " + to_wstring(carrinhoAtual.size()) + L"\nDescontos do Cliente:\n" + detalhesGerais + L"\n-------------------------------------------------\nTOTAL A PAGAR: R$ " + wstring(bufT) + L"\n\nConfirmar compra?";
                
                if(MessageBoxW(hwnd, msg.c_str(), L"Checkout", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    wstring erroDB;
                    if (db_vendas.finalizarVendaCarrinho(_wtoi(c), _wtoi(v), totalGeral, w_para_s(m), carrinhoAtual, erroDB)) {
                        MessageBoxW(hwnd, L"Venda concluída com sucesso!", L"Sucesso", MB_OK);
                        carrinhoAtual.clear(); SendMessage(hListCarrinho, LB_RESETCONTENT, 0, 0); 
                        AtualizarTabelasGlobais();
                    } else MessageBoxW(hwnd, (L"Erro de SQL:\n" + erroDB).c_str(), L"Falha", MB_OK | MB_ICONERROR);
                }
            }
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nC) {
    INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_WIN95_CLASSES}; InitCommonControlsEx(&icex);
    WNDCLASSW wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = L"MasterApp"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowW(L"MasterApp", L"Livraria do Victor - Sistema ERP Integrado", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1050, 550, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nC);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}