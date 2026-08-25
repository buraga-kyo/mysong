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

#include "tui/tokens.hpp"
#include "tui/transporte.hpp"

namespace mysong::tui {

namespace {

ftxui::Element pinta(const std::string& texto, std::string_view token) {
  const tokens::Triade c = tokens::rgb(token);
  return ftxui::text(texto) |
         ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
}

// apara — a cadeia em `largura` collunhas, contando CODEPOINTS e não bytes. Sem
// isto, um titulo com acentos sahiria mais curto do que a conta diz e a tabella
// perderia o alinhamento das columnas.
std::string apara(const std::string& crua, std::size_t largura) {
  std::string feita;
  std::size_t contadas = 0;
  for (std::size_t i = 0; i < crua.size(); ++i) {
    if ((static_cast<unsigned char>(crua[i]) & 0xC0) != 0x80) {
      if (contadas == largura) break;
      ++contadas;
    }
    feita += crua[i];
  }
  while (contadas++ < largura) feita += ' ';
  return feita;
}

}  // namespace

ftxui::Element elemento_da_barra(const Navegador& navegador) {
  // A ordem é a do mockup, e ella não muda com a secção: barra que se reordena
  // faz o dedo do operador errar o alvo que já sabia de memoria.
  const std::pair<Secao, const char*> degraus[] = {
      {Secao::Artistas, " ARTISTS "},
      {Secao::Albuns, " ALBUMS  "},
      {Secao::Faixas, " TRACKS  "},
      {Secao::Busca, " SEARCH  "},
  };
  std::vector<ftxui::Element> linhas;
  for (const auto& [degrau, rotulo] : degraus) {
    const bool aqui = navegador.secao() == degrau;
    ftxui::Element linha = pinta(rotulo, aqui ? tokens::text_bright
                                              : tokens::text_muted);
    if (aqui) {
      const tokens::Triade fundo = tokens::rgb(tokens::v700);
      linha = linha | ftxui::bgcolor(
                          ftxui::Color::RGB(fundo.r, fundo.g, fundo.b));
    }
    linhas.push_back(std::move(linha));
  }
  return ftxui::vbox(std::move(linhas));
}

ftxui::Element elemento_da_tabella(const Navegador& navegador,
                                   std::size_t primeira, std::size_t altura,
                                   std::size_t largura) {
  if (altura == 0 || largura == 0) return ftxui::text("");
  const std::vector<Linha>& vista = navegador.vista();
  if (vista.empty())
    return pinta("  (nada aqui: varra o acervo, ou baixe uma faixa)",
                 tokens::text_faint);

  // As tres columnas fixas: numero, tempo, e o que sobra para o titulo. O tempo
  // e o numero são de largura conhecida, e por isso o titulo é que cede.
  const std::size_t larg_num = 4, larg_tempo = 7;
  const std::size_t larg_titulo =
      largura > larg_num + larg_tempo + 2 ? largura - larg_num - larg_tempo - 2 : 1;

  std::vector<ftxui::Element> linhas;
  const std::size_t fim = std::min(primeira + altura, vista.size());
  for (std::size_t i = primeira; i < fim; ++i) {
    const Linha& linha = vista[i];
    const bool eleita = i == navegador.eleito();
    const std::string numero =
        linha.numero > 0 ? apara(std::to_string(linha.numero), larg_num)
                         : apara("", larg_num);
    const std::string tempo =
        linha.duracao > 0 ? apara(" " + mm_ss(linha.duracao), larg_tempo)
                          : apara("", larg_tempo);
    ftxui::Element pintada =
        pinta(numero + apara(linha.texto, larg_titulo) + tempo,
              eleita ? tokens::text_bright : tokens::text_muted);
    if (eleita) {
      const tokens::Triade fundo = tokens::rgb(tokens::v900);
      pintada = pintada | ftxui::bgcolor(
                              ftxui::Color::RGB(fundo.r, fundo.g, fundo.b));
    }
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
                                std::size_t collunas, std::size_t linhas) {
  if (collunas == 0 || linhas == 0) return ftxui::text("");
  if (capa.achada) {
    // Cada corrida vira UM elemento com a sua tinta. Não se passa a cadeia crua do
    // chafa: o FTXUI contaria os octetos do escape como LARGURA, e a capa esmagaria a
    // barra lateral e a tabella. Medi-o, e está registrado no tractado da capa.
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
    return ftxui::vbox(std::move(pintadas));
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
  return ftxui::vbox(std::move(pintadas)) | ftxui::border;
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
