// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA TABELLA — src/tui/tabella.cpp
// ══════════════════════════════════════════════════════════════════════════
// A pintura. Vale a regra do cabeçalho: pinta e sahe.
//
// DOMÍNIO ......... o Navegador por leitura, e a geometria.
// CONTRA-DOMÍNIO .. elementos do FTXUI.
// INVARIANTE ...... funcção alguma d'aqui muta o navegador. O parametro é
//                   `const&`, e o compilador guarda a regra.
// Q.E.D. .......... a decisão toda vive no navegador, e é lá que se prova; aqui
//                   sómente se traduz estado em tinta.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/tabella.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "tui/rato.hpp"
#include "tui/tokens.hpp"
#include "tui/transporte.hpp"

namespace mysong::tui {

namespace {

ftxui::Element pinta(const std::string& texto, std::string_view token) {
  const tokens::Triade c = tokens::rgb(token);
  return ftxui::text(texto) |
         ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
}

// cortar — a cadeia nos primeiros `largura` CODEPOINTS, e não bytes nem
// collunhas: o glypho largo (CJK, emoji) conta por um valendo duas. Sem a
// conta por codepoint, um titulo com acentos sahiria mais curto do que a conta
// diz e a tabella perderia o alinhamento das columnas.
//
// NÃO enche o que sobra, e o enchimento é que ficou no `apara`: quem põe caret
// no fim do texto quer o corte nú, que espaço á direita empurraria o caret uma
// collunha para lá do que se escreveu.
//
// Devolve tambem, por `deixadas`, quantas contou: quem enche o que sobra já não
// tem de tornar a percorrer a cadeia para o saber, e a volta pelo UTF-8 fica
// n'este logar só, que foi a razão de se partir o `apara` em dous.
std::string cortar(const std::string& crua, std::size_t largura,
                   std::size_t* deixadas = nullptr) {
  std::string feita;
  std::size_t contadas = 0;
  for (std::size_t i = 0; i < crua.size(); ++i) {
    if ((static_cast<unsigned char>(crua[i]) & 0xC0) != 0x80) {
      if (contadas == largura) break;
      ++contadas;
    }
    feita += crua[i];
  }
  if (deixadas != nullptr) *deixadas = contadas;
  return feita;
}

// apara — o corte, enchido de espaços até `largura`. É o enchimento que alinha
// as columnas da tabella, e é por isso que elle existe.
std::string apara(const std::string& crua, std::size_t largura) {
  std::size_t contadas = 0;
  std::string feita = cortar(crua, largura, &contadas);
  while (contadas++ < largura) feita += ' ';
  return feita;
}

}  // namespace

ftxui::Element caret_do_campo() {
  // Espaço, e não cadeia vazia: o cursor pousa no `x_min` da caixa d'este nó, e
  // nó de largura zero não tem caixa que sirva de endereço.
  return ftxui::text(" ") | ftxui::focusCursorBar;
}

ftxui::Element elemento_da_tabella(const Navegador& navegador,
                                   std::size_t primeira, std::size_t altura,
                                   std::size_t largura,
                                   const std::string& tocando,
                                   std::vector<ftxui::Box>* caixas) {
  // Limpa-se á entrada, e não sómente nos ramos que pintam linhas: sahida
  // antecipada que deixasse as caixas do quadro anterior faria o clique
  // acertar linhas que já não estão na tela.
  if (caixas != nullptr) caixas->clear();
  if (altura == 0 || largura == 0) return ftxui::text("");
  const std::vector<Linha>& vista = navegador.vista();
  if (vista.empty()) {
    // O recado do vazio é POR SECÇÃO. Um recado só dizia «varra o acervo» dentro de
    // uma lista de faixas escolhidas á mão, que é conselho que não serve para nada
    // e manda o operador ao logar errado.
    const char* recado = "  (nada aqui: varra o acervo, ou baixe uma faixa)";
    switch (navegador.secao()) {
      case Secao::Rois:
        recado = "  (lista alguma ainda: `c` cria uma)";
        break;
      case Secao::NoRol:
        recado = "  (lista vazia: elege uma faixa no acervo e tecla `a`)";
        break;
      case Secao::Rede:
        recado = "  (nada achado: `s` pergunta outra vez)";
        break;
      case Secao::Lista:
        recado = "  (lista alguma lida: `I` cola a URL de uma do Spotify)";
        break;
      case Secao::Busca:
        // As MINHAS MÚSICAS abrem com termo VAZIO (issue #93): não havendo
        // termo, quem está vazio é o acervo, e culpar o termo mandaria o
        // operador procurar erro de escripta que elle não commetteu.
        if (!navegador.termo().empty()) recado = "  (nada casa com esse termo)";
        break;
      case Secao::Artistas:
      case Secao::Albuns:
      case Secao::Faixas:
        break;
    }
    return pinta(recado, tokens::text_faint);
  }

  // As columnas fixas: numero, tempo, o AUTOR quando ha, e o que sobra para o
  // titulo. O tempo e o numero são de largura conhecida, e por isso o titulo cede.
  //
  // A columna do autor apparece pelo DADO, e não pela secção: havendo linha com
  // autor na fatia á vista, ella abre-se para todas as linhas d'essa fatia. Por
  // linha, e não por fatia, ella desalinharia as columnas de baixo com as de cima,
  // que é o defeito que faz a tabella parecer quebrada.
  const std::size_t fim_da_fatia = std::min(primeira + altura, vista.size());
  bool ha_autor = false;
  for (std::size_t i = primeira; i < fim_da_fatia; ++i)
    if (!vista[i].autor.empty()) ha_autor = true;
  const std::size_t larg_num = 4, larg_tempo = 7;
  const std::size_t larg_autor =
      ha_autor && largura >= 40 ? std::min<std::size_t>(24, largura / 4) : 0;
  const std::size_t fixas = larg_num + larg_tempo + larg_autor + 2;
  const std::size_t larg_titulo = largura > fixas ? largura - fixas : 1;

  std::vector<ftxui::Element> linhas;
  // Dimensiona-se ANTES do laço, pela razão da barra: o `reflect` guarda
  // referencia, e realloque no meio do quadro deixá-la-hia pendurada.
  // A subtracção vae GUARDADA, como a do transporte: em std::size_t tirar mais
  // do que ha dá numero enorme, e o `assign` tentaria armar bilhões de caixas.
  // Hoje o caso não chega aqui, que a `primeira_a_mostrar` o impede; mas esta
  // funcção é publica, e o laço de baixo já era tolerante ao mesmo engano.
  if (caixas != nullptr)
    caixas->assign(fim_da_fatia > primeira ? fim_da_fatia - primeira : 0,
                   caixa_por_pintar());
  for (std::size_t i = primeira; i < fim_da_fatia; ++i) {
    const Linha& linha = vista[i];
    const bool eleita = i == navegador.eleito();
    // O que SÔA casa-se pela CHAVE, que nas secções de faixa é o caminho do
    // arquivo. Nas outras a chave é nome ou id, e ahi nada casa, que é o que se
    // quer: album algum «toca». O «▶» toma o logar do numero, e não uma columna
    // nova: columna nova empurraria o titulo e desalinharia a tabella inteira
    // sómente porque alguma cousa sôa.
    const bool soa = !tocando.empty() && linha.chave == tocando;
    const std::string numero =
        soa ? apara(" \u25b6", larg_num)
        : linha.numero > 0 ? apara(std::to_string(linha.numero), larg_num)
                           : apara("", larg_num);
    const std::string tempo =
        linha.duracao > 0 ? apara(" " + mm_ss(linha.duracao), larg_tempo)
                          : apara("", larg_tempo);
    const std::string autor =
        larg_autor == 0 ? std::string() : apara(" " + linha.autor, larg_autor);
    ftxui::Element pintada =
        pinta(numero + apara(linha.texto, larg_titulo) + autor + tempo,
              soa       ? tokens::glow_core
              : eleita  ? tokens::text_bright
                        : tokens::text_muted);
    if (eleita) {
      const tokens::Triade fundo = tokens::rgb(tokens::v900);
      pintada = pintada | ftxui::bgcolor(
                              ftxui::Color::RGB(fundo.r, fundo.g, fundo.b));
    }
    if (caixas != nullptr)
      pintada = pintada | ftxui::reflect((*caixas)[i - primeira]);
    linhas.push_back(std::move(pintada));
  }
  return ftxui::vbox(std::move(linhas));
}

ftxui::Element elemento_da_letra(const std::vector<nucleo::LinhaDaLetra>& linhas,
                                 int corrente, std::size_t altura,
                                 std::size_t largura) {
  if (altura == 0 || largura == 0) return ftxui::text("");
  if (linhas.empty())
    return pinta(apara("  (sem letra para esta faixa)", largura),
                 tokens::text_faint);

  // A corrente vae no MEIO da janella, e não no alto: o operador lê o que vem, e
  // não sómente o que passou. Fica no alto sómente no principio da musica, e no
  // fim quando já não ha o que vir.
  const std::size_t meio = altura / 2;
  const int alvo = corrente < 0 ? 0 : corrente;
  std::size_t primeira = 0;
  if (static_cast<std::size_t>(alvo) > meio) primeira = alvo - meio;
  if (primeira + altura > linhas.size())
    primeira = linhas.size() > altura ? linhas.size() - altura : 0;

  std::vector<ftxui::Element> pintadas;
  const std::size_t fim = std::min(primeira + altura, linhas.size());
  for (std::size_t i = primeira; i < fim; ++i) {
    const bool esta = static_cast<int>(i) == corrente;
    ftxui::Element linha =
        pinta(apara("  " + linhas[i].texto, largura),
              esta ? tokens::text_bright : tokens::text_faint);
    if (esta) linha = linha | ftxui::bold;
    pintadas.push_back(std::move(linha));
  }
  return ftxui::vbox(std::move(pintadas));
}

ftxui::Element elemento_da_capa(const nucleo::CapaPintada& capa,
                                std::size_t collunas, std::size_t linhas,
                                ftxui::Box* caixa) {
  // Esvazia-se á entrada: terminal apertado não mostra capa alguma, e a caixa
  // do quadro anterior deixaria o clique a pausar sobre a tabella.
  if (caixa != nullptr) *caixa = caixa_por_pintar();
  if (collunas == 0 || linhas == 0) return ftxui::text("");
  const auto lembrar = [caixa](ftxui::Element pintada) {
    return caixa == nullptr ? pintada : pintada | ftxui::reflect(*caixa);
  };
  if (capa.achada) {
    // Cada corrida vira UM elemento com a sua tinta. Não se passa a cadeia crua do
    // chafa: o FTXUI contaria os octetos do escape como LARGURA, e a capa esmagaria a
    // pauta e o painel. Medi-o, e está registrado no tractado da capa.
    std::vector<ftxui::Element> pintadas;
    pintadas.reserve(capa.linhas.size());
    for (const std::vector<nucleo::Corrida>& linha : capa.linhas) {
      std::vector<ftxui::Element> corridas;
      corridas.reserve(linha.size());
      for (const nucleo::Corrida& corrida : linha) {
        ftxui::Element pedaco = ftxui::text(corrida.texto);
        if (corrida.r_frente >= 0)
          pedaco = pedaco | ftxui::color(ftxui::Color::RGB(
                                corrida.r_frente, corrida.g_frente,
                                corrida.b_frente));
        if (corrida.r_fundo >= 0)
          pedaco = pedaco | ftxui::bgcolor(ftxui::Color::RGB(
                                corrida.r_fundo, corrida.g_fundo,
                                corrida.b_fundo));
        corridas.push_back(std::move(pedaco));
      }
      pintadas.push_back(ftxui::hbox(std::move(corridas)));
    }
    return lembrar(ftxui::vbox(std::move(pintadas)));
  }

  // O MARCADOR: uma nota musical no meio de um quadro de orla, com os tokens d'esta
  // Casa. Não é enfeite: album sem capa mostra que NÃO TEM, e não um buraco que o
  // operador tomaria por falha da tela.
  std::vector<ftxui::Element> pintadas;
  const std::size_t meio = linhas / 2;
  for (std::size_t l = 0; l < linhas; ++l) {
    if (l == meio) {
      const std::size_t esquerda = collunas > 1 ? (collunas - 1) / 2 : 0;
      pintadas.push_back(pinta(std::string(esquerda, ' ') + "\u266b",
                               tokens::text_faint));
    } else {
      pintadas.push_back(pinta(std::string(collunas, ' '), tokens::inset));
    }
  }
  return lembrar(ftxui::vbox(std::move(pintadas)) | ftxui::border);
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
