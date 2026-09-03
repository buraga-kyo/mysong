// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SALA — src/tui/sala.cpp
// ══════════════════════════════════════════════════════════════════════════
// A lavra das peças que o sala.hpp declara. Compõe, e sahe.
//
// DOMÍNIO ......... os retractos e a geometria.
// CONTRA-DOMÍNIO .. cadeias e elementos do FTXUI.
// INVARIANTE ...... funcção alguma d'aqui lê o mundo: nem banco, nem relogio.
// Q.E.D. .......... a bateria arma os retractos á mão e afere-a em papel.
// ══════════════════════════════════════════════════════════════════════════
#include <algorithm>
#include <filesystem>
#include <string_view>
#include <utility>
#include <vector>

#include <ftxui/screen/string.hpp>

#include "tui/sala.hpp"
#include "tui/tabella.hpp"
#include "tui/tokens.hpp"

namespace mysong::tui {
namespace {
// kMarcadorLinhas — a área do marcador. Seis, e não o tecto: a capa de 16 por
// 9 sahe em cerca d'onze linhas n'um painel de 39, e moldura vazia de vinte
// diria «não ha capa» mais alto do que o painel diz a musica.
constexpr std::size_t kMarcadorLinhas = 6;

// pinta — o texto na tinta do token. Côr crua não entra n'esta obra.
ftxui::Element pinta(const std::string& texto, std::string_view token) {
  const tokens::Triade c = tokens::rgb(token);
  return ftxui::text(texto) | ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
}

// substantivo_da — o que se conta, em caixa alta. Singular SEM o `s`.
const char* substantivo_da(Especie especie, bool um) {
  switch (especie) {
    case Especie::Artistas: return um ? "ARTISTA" : "ARTISTAS";
    case Especie::Albuns: return um ? "ÁLBUM" : "ÁLBUNS";
    case Especie::Listas: return um ? "LISTA" : "LISTAS";
    case Especie::Achados: return um ? "ACHADO" : "ACHADOS";
    case Especie::Faixas: break;
  }
  return um ? "FAIXA" : "FAIXAS";
}

// chip — o modo aceso ou apagado, e PRESENTE nos dous casos: chip que sommisse
// mudaria a largura da linha da conta a cada tecla, e a tela saltaria sozinha.
ftxui::Element chip(const std::string& texto, bool aceso) {
  if (!aceso) return pinta(texto, tokens::text_faint);
  const tokens::Triade fundo = tokens::rgb(tokens::v700);
  return pinta(texto, tokens::text_bright) |
         ftxui::bgcolor(ftxui::Color::RGB(fundo.r, fundo.g, fundo.b));
}
// Os numeros da sala nova (issue #102). O LIMIAR do painel é de CEM collunhas
// de TELA, e não de largura util: a tela nova não leva orla, e cem é o numero
// que a issue diz. Metade de cem é cincoenta, e o painel de trinta com a pauta
// de quarenta cabem n'ellas com folga.
constexpr std::size_t kLimiarDoPainel = 100, kPainelMinimo = 30;
constexpr std::size_t kPautaMinima = 40;
// O espectro não desce de seis linhas, e a capa cede-lhe o logar antes d'elle
// encolher: espectro de tres linhas não é serie de dados, é enfeite.
constexpr std::size_t kEspectroMinimo = 6, kCapaPorCento = 45;
// A chapa cede o logar á pauta quando ella ficaria com menos de tres linhas: a
// pauta é onde se navega, e a chapa diz sómente onde se está.
constexpr std::size_t kPautaLinhasMinimas = 3;

// reparte_o_corpo — as duas metades, dado o alto e a altura que sobraram. Sahe
// á parte da conta do alto por ser a lavra que a tela ESTREITA muda: abaixo do
// limiar não ha painel algum, e a pauta toma a tela toda, como hoje.
void reparte_o_corpo(Sala& sala, std::size_t largura, std::size_t alto,
                     std::size_t corpo) {
  if (corpo == 0) return;
  std::size_t do_painel = 0;
  if (largura >= kLimiarDoPainel && corpo >= kEspectroMinimo) {
    // METADE e METADE, que é o que elle pediu. A collunha do divisor sahe da
    // esquerda, donde em largura impar a pauta fica uma mais estreita.
    do_painel = largura / 2;
    if (do_painel < kPainelMinimo ||
        largura - do_painel - 1 < kPautaMinima)
      do_painel = 0;
  }
  const std::size_t da_pauta =
      do_painel == 0 ? largura : largura - do_painel - 1;
  const bool ha_chapa = corpo >= kPautaLinhasMinimas + 1;
  if (ha_chapa) sala.chapa = {0, alto, da_pauta, 1};
  sala.pauta = {0, ha_chapa ? alto + 1 : alto, da_pauta,
                ha_chapa ? corpo - 1 : corpo};
  if (do_painel == 0) return;
  sala.divisor = {da_pauta, alto, 1, corpo};
  sala.painel = {da_pauta + 1, alto, do_painel, corpo};
  // O TECTO da capa: quarenta e cinco por cento do painel, e nunca tanto que
  // deixe o espectro abaixo do minimo d'elle.
  const std::size_t tecto =
      std::min(corpo * kCapaPorCento / 100, corpo - kEspectroMinimo);
  sala.capa = {sala.painel.x, alto, do_painel, tecto};
  sala.espectro = {sala.painel.x, alto + tecto, do_painel, corpo - tecto};
}

}  // namespace

std::string texto_da_duracao(int segundos) {
  if (segundos <= 0) return {};
  if (segundos < 60) return std::to_string(segundos) + "s";
  const int minutos = segundos / 60;
  if (minutos < 60) return std::to_string(minutos) + "min";
  // Zero á esquerda nos minutos: «1h5» lê-se hora e cinco HORAS.
  const int resto = minutos % 60;
  return std::to_string(minutos / 60) + "h" + (resto < 10 ? "0" : "") +
         std::to_string(resto);
}

std::string texto_da_conta(std::size_t quantas, Especie especie, int duracao) {
  std::string feita =
      std::to_string(quantas) + " " + substantivo_da(especie, quantas == 1);
  const std::string tempo = texto_da_duracao(duracao);
  if (!tempo.empty()) feita += ", " + tempo;
  return feita;
}

bool chave_e_caminho(Secao secao) {
  switch (secao) {
    case Secao::Faixas:
    case Secao::Busca:
    case Secao::NoRol: return true;
    case Secao::Artistas:
    case Secao::Albuns:
    case Secao::Rede:
    case Secao::Rois:
    case Secao::Lista: break;
  }
  return false;
}

Especie especie_da_secao(Secao secao) {
  switch (secao) {
    case Secao::Artistas: return Especie::Artistas;
    case Secao::Albuns: return Especie::Albuns;
    case Secao::Rois: return Especie::Listas;
    case Secao::Rede: return Especie::Achados;
    case Secao::Faixas:
    case Secao::Busca:
    case Secao::NoRol:
    case Secao::Lista: break;
  }
  return Especie::Faixas;
}

std::string onde_da_chapa(Secao secao, const std::vector<std::string>& trilha,
                          const std::string& nome_do_catalogo) {
  // A palavra da ABA primeiro, e os degraus de dentro depois: a chapa diz o
  // CAMINHO, e não sómente o ultimo degrau, que era o que o cabeçalho velho
  // dizia. Quem entrou n'um album por um artista lê os dous, e sabe voltar.
  //
  // Degrau de nome VAZIO conta por fóra: o mp3 sem etiqueta de album entra
  // n'um album que se chama nada, e a chapa sahiria com um «▸» sem palavra.
  std::string dito;
  switch (secao) {
    case Secao::Artistas:
    case Secao::Albuns:
    case Secao::Faixas:
    case Secao::Busca: dito = "MY SONG"; break;
    case Secao::Rois:
    case Secao::NoRol: dito = "PLAYLISTS"; break;
    case Secao::Rede: dito = "DOWNLOAD"; break;
    case Secao::Lista:
      dito = "DOWNLOAD \u25b8 SPOTIFY";
      if (!nome_do_catalogo.empty()) dito += " \u25b8 " + nome_do_catalogo;
      return dito;
  }
  for (const std::string& degrau : trilha)
    if (!degrau.empty()) dito += " \u25b8 " + degrau;
  return dito;
}

Sala sala_da_tela(std::size_t largura, std::size_t altura, bool campo_aberto) {
  Sala sala;
  if (largura == 0 || altura == 0) return sala;
  sala.cabecalho = {0, 0, largura, 1};
  if (altura < 2) return sala;
  sala.trilho = {0, 1, largura, 1};
  std::size_t alto = 2;  // a primeira linha ainda por repartir
  if (campo_aberto && altura > alto) {
    sala.campo = {0, alto, largura, 1};
    ++alto;
  }
  if (altura <= alto) return sala;
  // O rodapé cede o logar quando não sobraria linha alguma ao corpo: dizer a
  // tecla sem mostrar a lista é dar o caminho e fechar a porta.
  std::size_t baixo = altura;
  if (altura >= alto + 2) {
    sala.rodape = {0, altura - 1, largura, 1};
    baixo = altura - 1;
  }
  reparte_o_corpo(sala, largura, alto, baixo - alto);
  return sala;
}

Rectangulo espectro_abaixo_da(const Sala& sala, std::size_t linhas_da_capa) {
  if (sala.painel.vazio()) return {};
  const std::size_t tomadas = std::min(linhas_da_capa, sala.capa.altura);
  return {sala.painel.x, sala.painel.y + tomadas, sala.painel.largura,
          sala.painel.altura - tomadas};
}

Ficha ficha_da_faixa(const std::string& caminho, const std::string& titulo,
                     const std::string& artista, const std::string& album) {
  if (caminho.empty()) return {};
  Ficha ficha{titulo, artista, album};
  if (ficha.titulo.empty())
    ficha.titulo = std::filesystem::path(caminho).stem().string();
  return ficha;
}

ftxui::Element elemento_da_ficha(const Ficha& ficha, std::size_t largura) {
  if (largura == 0) return ftxui::text("");
  // As TRES linhas levam o token, e não sómente a primeira: as duas de baixo
  // estão vazias hoje, e linha vazia sem tinta é a que amanhã ganha texto e
  // sahe na côr de repouso sem que ninguem repare.
  if (ficha.titulo.empty())
    return ftxui::vbox({pinta("(nada toca)", tokens::text_faint),
                        pinta("", tokens::text_faint),
                        pinta("", tokens::text_faint)});
  return ftxui::vbox({pinta(ficha.titulo, tokens::text_bright) | ftxui::bold,
                      pinta(ficha.artista, tokens::text_primary),
                      pinta(ficha.album, tokens::text_muted)});
}

std::size_t linhas_da_arte(const nucleo::CapaPintada& capa, std::size_t tecto) {
  const std::size_t quer = capa.achada ? capa.linhas.size() : kMarcadorLinhas;
  return std::min(quer, tecto);
}

ftxui::Element elemento_da_arte(const nucleo::CapaPintada& capa,
                                std::size_t largura, std::size_t linhas) {
  // `emptyElement`, e NÃO `text("")`: o `text` do FTXUI pede sempre UMA linha,
  // ainda que nada escreva. Devolvendo-o, o painel sem capa pedia uma linha a
  // mais que a conta lhe deu, e o cinge da faixa do corpo aparava a ultima do
  // espectro em silencio. Medido n'um pty de vinte linhas.
  if (largura == 0 || linhas == 0) return ftxui::emptyElement();
  // O tecto vale tambem para a capa ACHADA. O `elemento_da_capa` pinta TODAS as
  // linhas que traz, e não olha o `linhas` que se lhe passa; a promessa do
  // `linhas_da_arte` é o minimo entre o que veio e o tecto. Sem este cinge,
  // capa mais alta que o tecto empurraria a ficha para fóra do painel.
  if (capa.achada)
    return elemento_da_capa(capa, largura, linhas) |
           ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN,
                       static_cast<int>(linhas));
  if (largura <= 2 || linhas <= 2) return ftxui::emptyElement();
  return elemento_da_capa(capa, largura - 2, linhas - 2);
}

// linha_da_conta — a conta e os chips. Elles CEDEM O LOGAR quando a linha não
// cabe, como a fita do transporte: chip aparado come o vão da capa e diz nada.
ftxui::Element linha_da_conta(const Colleccao& qual, std::size_t largura) {
  const bool repete = qual.repeticao != nucleo::Repeticao::Nenhuma;
  const bool uma = qual.repeticao == nucleo::Repeticao::Uma;
  const std::string conta =
      texto_da_conta(qual.quantas, qual.especie, qual.duracao);
  const std::string um = " \u21c4 EMBARALHAR ";
  const std::string dous = std::string(" \u21bb REPETIR: ") +
                           (!repete ? "NÃO" : uma ? "UMA" : "TODAS") + " ";
  std::vector<ftxui::Element> partes = {pinta(conta + "  ", tokens::text_body)};
  const int pede = ftxui::string_width(conta) + ftxui::string_width(um) +
                   ftxui::string_width(dous);
  // O CINCO são os vãos que o texto não conta: DOUS entre a capa pequena e o
  // que vae á direita d'ella, DOUS depois da conta, e UM entre os chips.
  if (largura >= kCapaPequena + 5 + static_cast<std::size_t>(pede)) {
    partes.push_back(chip(um, qual.embaralhado));
    partes.push_back(ftxui::text(" "));
    partes.push_back(chip(dous, repete));
  }
  return ftxui::hbox(std::move(partes));
}

ftxui::Element elemento_do_cabecalho(const Colleccao& colleccao,
                                     const nucleo::CapaPintada& capa,
                                     std::size_t largura) {
  if (largura == 0) return ftxui::text("");
  std::string risca;
  for (std::size_t c = 0; c < largura; ++c) risca += "\u2500";
  // O que sobra depois da capa pequena e do vão d'ella. O CINGE não é enfeite:
  // nome comprido punha a sua largura no `min_x` do meio, e o `flex_shrink_x`
  // do FTXUI nasce zero, d'onde o `hbox` cahe no encolhimento DURO e apara
  // TODOS os irmãos por egual, a barra e o painel inclusive, apesar de estes
  // pedirem largura EGUAL. Medido: nome de cento e vinte collunhas em cento e
  // cincoenta e seis levava a barra de nove a oito e o painel de trinta e nove
  // a trinta e dous. É a mesma mecanica dos chips, e a mesma cura.
  const std::size_t sobra =
      largura > kCapaPequena + 2 ? largura - kCapaPequena - 2 : 1;
  return ftxui::vbox(
      {ftxui::hbox({elemento_da_arte(capa, kCapaPequena, kCapaPequenaLinhas),
                    ftxui::text("  "),
                    ftxui::vbox({ftxui::text(""),
                                 pinta(colleccao.nome, tokens::text_heading) |
                                     ftxui::bold,
                                 ftxui::text(""),
                                 linha_da_conta(colleccao, largura),
                                 ftxui::text("")}) |
                        ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN,
                                    static_cast<int>(sobra))}) |
           ftxui::size(ftxui::HEIGHT, ftxui::EQUAL,
                       static_cast<int>(kCapaPequenaLinhas)),
       pinta(risca, tokens::line_dim)});
}

ftxui::Element elemento_do_painel(const Ficha& ficha, ftxui::Element arte,
                                  ftxui::Element baixo, std::size_t largura) {
  if (largura == 0) return ftxui::text("");
  // O fundo `panel` veste a collunna inteira, e é elle que a aparta do meio:
  // orla custaria duas collunhas, que n'este painel sahem da arte.
  const tokens::Triade fundo = tokens::rgb(tokens::panel);
  return ftxui::vbox({pinta("TOCANDO AGORA", tokens::text_heading) | ftxui::bold,
                      std::move(arte), elemento_da_ficha(ficha, largura),
                      std::move(baixo)}) |
         ftxui::bgcolor(ftxui::Color::RGB(fundo.r, fundo.g, fundo.b)) |
         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(largura));
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
