// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO TRANSPORTE — src/tui/transporte.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação da composição. Vale aqui a mesma regra do cabeçalho: cousa
// alguma d'este arquivo sabe o que é um Tocador. Se um dia souber, a prova do
// criterio C4 accusa, que ella busca os nomes proibidos por grep.
//
// DOMÍNIO ......... o Retracto e a largura.
// CONTRA-DOMÍNIO .. cadeias e elementos, deterministicos.
// INVARIANTE ...... funcção alguma d'aqui lança, e nenhuma divide sem antes
//                   provar o divisor: tempo de faixa vem do mpv, e o mpv
//                   entrega zero e não-numero antes de a faixa carregar.
// Q.E.D. .......... sendo tudo funcção de valores, a bateria afere o quadro
//                   contra alvo escripto á mão.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/transporte.hpp"

#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

#include "tui/arrowline.hpp"
#include "tui/tokens.hpp"

namespace mysong::tui {

// Os dous glifos da barra. Bloco cheio e bloco leve, e não espaço para o vazio:
// espaço com fundo pintado depende de o terminal honrar o fundo até á borda da
// célulla, e ha emulador que o não faz; glifo desenhado sahe sempre.
inline constexpr std::string_view kBarraCheia = "\u2588";
inline constexpr std::string_view kBarraVazia = "\u2591";

// Os glifos dos botões. Escrevem-se por PONTO DE CODIGO pela razão da fita, e
// ganham nome porque servem tambem de ENDEREÇO: é por elles que a caixa do rato
// (issue #95) acha o seu segmento, e não pela ordem em que a fita os junta.
inline constexpr std::string_view kPausar = "\u23f8";
inline constexpr std::string_view kTocar = "\u25b6";
inline constexpr std::string_view kAnterior = "\u23ee";
inline constexpr std::string_view kProxima = "\u23ed";


std::string mm_ss(double segundos) {
  // Tempo que não é tempo mostra-se como tal, e não como `00:00`: zero é uma
  // affirmação (a faixa está no principio), e o traço é a confissão de que a
  // Casa ainda não sabe. Confundir os dous faria a tela mentir no arranque.
  if (!std::isfinite(segundos) || segundos < 0.0) return "--:--";
  const long inteiro = static_cast<long>(segundos);
  const long minutos = inteiro / 60;
  const long resto = inteiro % 60;
  char molde[32];
  std::snprintf(molde, sizeof molde, "%02ld:%02ld", minutos, resto);
  return std::string(molde);
}

std::size_t enchimento(double posicao, double duracao, std::size_t largura) {
  if (largura == 0) return 0;
  // Duração que não presta dá barra vazia, e a guarda vem ANTES da divisão: o
  // mpv entrega duração zero enquanto a faixa carrega, e dividir alli daria
  // infinito, que o `round` converteria em numero qualquer.
  if (!std::isfinite(duracao) || duracao <= 0.0) return 0;
  if (!std::isfinite(posicao) || posicao <= 0.0) return 0;
  double razao = posicao / duracao;
  if (razao > 1.0) razao = 1.0;  // buscou-se para o fim, ou o mpv passou d'elle
  const double collunhas = std::round(razao * static_cast<double>(largura));
  const std::size_t cheias = static_cast<std::size_t>(collunhas);
  return cheias > largura ? largura : cheias;
}

namespace {

// pinta — texto com tinta de token, que é o que se repete em toda a barra.
ftxui::Element pinta(const std::string& texto, std::string_view token) {
  const tokens::Triade c = tokens::rgb(token);
  return ftxui::text(texto) | ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
}

// repete — n copias do glifo. O glifo é multibyte, donde std::string(n, ch) não
// serve: ella repetiria UM byte, e a barra sahiria em lixo.
std::string repete(std::string_view glifo, std::size_t n) {
  std::string feito;
  feito.reserve(glifo.size() * n);
  for (std::size_t i = 0; i < n; ++i) feito += glifo;
  return feito;
}

// Os pedaços da fita em elementos. A regra do DESIGN_SYSTEM manda que a côr da
// seta seja a côr do segmento que ella SEGUE, e a fita já a resolveu: aqui
// sómente se pinta o que ella diz.
ftxui::Element fita_em_elemento(const std::vector<Pedaco>& pedacos,
                                CaixasDoTransporte* caixas) {
  std::vector<ftxui::Element> partes;
  partes.reserve(pedacos.size() + 1);
  for (const Pedaco& pedaco : pedacos) {
    const tokens::Triade frente = tokens::rgb(pedaco.tinta);
    const tokens::Triade tras = tokens::rgb(pedaco.fundo);
    const auto vestir = [&](const std::string& texto) {
      return ftxui::text(texto) |
             ftxui::color(ftxui::Color::RGB(frente.r, frente.g, frente.b)) |
             ftxui::bgcolor(ftxui::Color::RGB(tras.r, tras.g, tras.b));
    };
    const std::size_t salto = pedaco.texto.find(kProxima);
    if (caixas != nullptr && !pedaco.juncao && salto != std::string::npos) {
      // O segmento dos DOUS saltos parte-se ao meio, em textos de EGUAL tinta e
      // egual fundo: as cellas sahem as mesmas, e cada glypho ganha caixa
      // propria. Caixa do pedaço inteiro não saberia dizer em qual dos dous o
      // dedo pousou, e partir a FITA em dous segmentos metteria entre elles um
      // glypho de junção, que é mudar o desenho para achar o dedo.
      partes.push_back(vestir(pedaco.texto.substr(0, salto)) |
                       ftxui::reflect(caixas->anterior));
      partes.push_back(vestir(pedaco.texto.substr(salto)) |
                       ftxui::reflect(caixas->proxima));
      continue;
    }
    ftxui::Element parte = vestir(pedaco.texto);
    if (caixas != nullptr && !pedaco.juncao &&
        (pedaco.texto.find(kPausar) != std::string::npos ||
         pedaco.texto.find(kTocar) != std::string::npos))
      parte = parte | ftxui::reflect(caixas->pausa);
    partes.push_back(std::move(parte));
  }
  return ftxui::hbox(std::move(partes));
}

// rotulo_dos_modos — o que a fita diz dos dous modos, e cadeia VAZIA quando os
// dous estão desligados. ASCII curto, e não o glifo bonito: a Fita conta
// CODEPOINTS, e os emoji de embaralhar e de repetir occupam DUAS collunhas no
// terminal; a linha transbordaria sem que conta alguma o accusasse.
std::string rotulo_dos_modos(const Retracto& retracto) {
  std::string dito;
  if (retracto.embaralhado) dito = "emb";
  if (retracto.repeticao != nucleo::Repeticao::Nenhuma) {
    if (!dito.empty()) dito += ' ';
    dito += "rep ";
    dito += nucleo::nome_da_repeticao(retracto.repeticao);
  }
  return dito;
}

}  // namespace

// A fita dos botões e do estado. Sentido DEXTRA sómente: misturar os dous
// lavraria o losango que a regra proscreve, e a Fita já o torna inexprimivel.
// Os fundos descem pela rampa, do acento cardeal ao fundo do painel, que é a
// leitura da esquerda para a direita.
//
// Devolve a FITA, e não o elemento: quem compõe precisa da largura que ella pede
// ANTES de repartir o que sobra, e a fita é quem a sabe dizer.
Fita fita_dos_botoes(const Retracto& retracto, bool com_modos) {
  const bool tocando = retracto.estado == nucleo::Estado::Tocando;
  Fita fita(Sentido::Dextra);
  fita.junta({" " + std::string(tocando ? kPausar : kTocar) + " ",
              tokens::v500, tokens::base});
  fita.junta({" " + std::string(kAnterior) + " " + std::string(kProxima) + " ",
              tokens::v700, tokens::text_bright});
  fita.junta({" " + std::string(nucleo::nome_do_estado(retracto.estado)) + " ",
              tokens::v900, tokens::text_bright});
  // Os dous modos, e SÓMENTE quando ha modo ligado: fita que dissesse «emb:
  // não» gastaria collunhas para dizer que nada ha. Com os dous desligados a
  // fita sae egual á de sempre, byte a byte, e a prova que já existe o afere.
  const std::string modos = com_modos ? rotulo_dos_modos(retracto) : std::string();
  if (!modos.empty())
    fita.junta({" " + modos + " ", tokens::v950, tokens::text_primary});
  return fita;
}

std::string linha_da_barra(const Retracto& retracto, std::size_t largura) {
  const std::size_t cheias = enchimento(retracto.posicao, retracto.duracao, largura);
  std::string linha;
  linha.reserve(largura * 3);
  for (std::size_t c = 0; c < largura; ++c)
    linha += (c < cheias) ? kBarraCheia : kBarraVazia;
  return linha;
}

ftxui::Element elemento_do_transporte(const Retracto& retracto,
                                      std::size_t largura,
                                      CaixasDoTransporte* caixas) {
  // Esvazia-se á entrada, e antes de toda sahida antecipada: linha que se não
  // pintou não ha de deixar caixa do quadro anterior a apanhar cliques.
  if (caixas != nullptr) *caixas = CaixasDoTransporte();
  if (largura == 0) return ftxui::text("");

  const std::string relogio =
      " " + mm_ss(retracto.posicao) + " / " + mm_ss(retracto.duracao) + " ";
  const std::string som = "vol " + std::to_string(retracto.volume) + "% ";

  // A barra toma o que sobra, e nunca menos que uma collunha. A subtracção é
  // GUARDADA: em std::size_t, tirar mais do que ha dá numero enorme, e a barra
  // tentaria pintar bilhões de collunhas em vez de encolher.
  // A largura da fita PERGUNTA-SE Á FITA. Estava chumbada em quatorze, e a fita
  // pede vinte e uma: a linha transbordava, o FTXUI aparava o fim, e o que se
  // perdia era o espaço entre o relogio e o volume. Numero chumbado alli é o
  // defeito, e não a sua magnitude; quem sabe a largura é quem a compõe.
  // Os dous modos CEDEM O LOGAR quando a linha não cabe, e sahem inteiros. Foi
  // medido: o FTXUI não apara sómente a barra, encolhe todo elemento da linha, e
  // a trinta collunhas a fita sahia «Toc emb r». «emb r» diz menos que nada, e o
  // que fica sem elle é a fita que o operador já conhece.
  Fita fita = fita_dos_botoes(retracto, true);
  std::size_t reservado =
      fita.largura_exigida() + 1 + relogio.size() + som.size();
  if (largura <= reservado) {
    fita = fita_dos_botoes(retracto, false);
    reservado = fita.largura_exigida() + 1 + relogio.size() + som.size();
  }
  const std::size_t larg_barra = largura > reservado ? largura - reservado : 1;
  const std::size_t cheias =
      enchimento(retracto.posicao, retracto.duracao, larg_barra);

  // As duas metades da barra reflectem-se á parte, e a união d'ellas é que dá a
  // barra inteira: n'um hbox aninhado o FTXUI reparte a sobra por outro grupo, e
  // o que se ganharia em uma linha pagar-se-hia em desenho torto na tela
  // apertada. A união sabe tratar a metade de largura zero, que é o principio e
  // o fim de toda faixa.
  ftxui::Element cheia = pinta(repete(kBarraCheia, cheias), tokens::v500);
  ftxui::Element vazia =
      pinta(repete(kBarraVazia, larg_barra - cheias), tokens::inset);
  if (caixas != nullptr) {
    cheia = cheia | ftxui::reflect(caixas->barra_cheia);
    vazia = vazia | ftxui::reflect(caixas->barra_vazia);
  }

  return ftxui::hbox({
      fita_em_elemento(fita.compor(), caixas),
      ftxui::text(" "),
      std::move(cheia),
      std::move(vazia),
      pinta(relogio, tokens::text_bright),
      pinta(som, tokens::text_muted),
  });
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
