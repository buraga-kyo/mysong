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
    // esquerda, donde em largura PAR a pauta fica uma mais estreita (a 120
    // dá 59 e 60) e em largura IMPAR as duas ficam eguaes (a 167 dão 83).
    do_painel = largura / 2;
    // As duas guardas são CINTO DE SEGURANÇA, e hoje nenhuma pode correr: com
    // o limiar em cem, a metade é sempre de cincoenta ou mais, e a pauta de
    // quarenta e nove ou mais. Ficam para o dia em que o limiar baixar, que
    // baixá-lo sem ellas poria painel de vinte collunhas na tela.
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

std::string texto_da_chapa(const Chapa& chapa) {
  std::string dito = chapa.onde;
  if (!dito.empty()) dito += ", ";
  dito += texto_da_conta(chapa.quantas, chapa.especie, chapa.duracao);
  // A VISTA sómente onde ella se cycla: chapa que dissesse «FAIXAS» n'uma
  // lista de listas prometteria uma tecla que alli não faz cousa alguma.
  if (!chapa.vista.empty()) dito += ", " + chapa.vista;
  return dito;
}

// esquerda_da_chapa — o que se pinta á ESQUERDA: o texto, e as encommendas
// logo depois d'elle quando as ha. Serve á pintura e á conta do espaço, para
// que as duas leiam a MESMA cadeia e não divirjam de uma collunha.
static std::string esquerda_da_chapa(const Chapa& chapa) {
  std::string dita = " " + texto_da_chapa(chapa);
  if (!chapa.conselho.empty()) dita += "  " + chapa.conselho;
  if (!chapa.encommendas.empty()) dita += "  " + chapa.encommendas;
  return dita;
}

std::size_t espaco_do_recado(const Chapa& chapa, std::size_t largura) {
  const std::size_t gasto =
      static_cast<std::size_t>(ftxui::string_width(esquerda_da_chapa(chapa)));
  // Duas collunhas de folga: uma de vão entre o texto e o recado, e a do
  // espaço que o recado leva no fim para não encostar na borda.
  return largura > gasto + 2 ? largura - gasto - 2 : 0;
}

ftxui::Element elemento_da_chapa(const Chapa& chapa, std::size_t largura) {
  if (largura == 0) return ftxui::emptyElement();
  const tokens::Triade fundo = tokens::rgb(tokens::panel_hi);
  std::vector<ftxui::Element> partes = {
      pinta(" " + texto_da_chapa(chapa), tokens::text_heading) | ftxui::bold};
  // O CONSELHO em glow_soft, logo depois da conta: a pauta vazia é o unico
  // estado em que a chapa tem de CHAMAR o dedo, e o glow é d'esta Casa o que
  // chama. Vae antes das encommendas, que ellas correm com a lista cheia.
  if (!chapa.conselho.empty())
    partes.push_back(pinta("  " + chapa.conselho, tokens::glow_soft));
  // As ENCOMMENDAS em data2, que é o amarello do Poente Contido: ellas são
  // ESTADO em curso, e estado é acento. Vão logo á direita do texto, e nunca
  // no fim: alli o primeiro aviso comprido comia-lhes o logar.
  if (!chapa.encommendas.empty())
    partes.push_back(pinta("  " + chapa.encommendas, tokens::data2));
  const std::size_t sobra = espaco_do_recado(chapa, largura);
  if (!chapa.recado.empty() && sobra > 0) {
    partes.push_back(ftxui::filler());
    partes.push_back(pinta(chapa.recado + " ", tokens::glow_soft) |
                     ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN,
                                 static_cast<int>(sobra + 1)));
  }
  return ftxui::hbox(std::move(partes)) |
         ftxui::bgcolor(ftxui::Color::RGB(fundo.r, fundo.g, fundo.b)) |
         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(largura));
}

ftxui::Element elemento_do_divisor(std::size_t altura) {
  if (altura == 0) return ftxui::emptyElement();
  // O traço PESADO vertical, e não o `separator` de fabrica: aquelle pinta na
  // côr herdada, e esta Casa não deixa côr por decretar.
  std::vector<ftxui::Element> cellas;
  cellas.reserve(altura);
  for (std::size_t l = 0; l < altura; ++l)
    cellas.push_back(pinta("\u2503", tokens::line_dim));
  return ftxui::vbox(std::move(cellas));
}

ftxui::Element elemento_do_painel(ftxui::Element arte, ftxui::Element baixo,
                                  std::size_t largura) {
  if (largura == 0) return ftxui::emptyElement();
  // O fundo `panel` veste a collunna inteira, e é elle que a aparta da pauta
  // por dentro; por fóra aparta-a o divisor, que é collunha propria.
  const tokens::Triade fundo = tokens::rgb(tokens::panel);
  // A ARTE vae CENTRADA, que é o que a issue pede. O chafa guarda a proporção,
  // d'onde a capa quadrada n'um painel largo sahe mais estreita que elle: sem
  // o centro ella ficava encostada á esquerda, com o vão todo de um lado só.
  // O `hcenter` mede o que a arte pediu, e não o que o painel tem, donde a
  // capa que enche a largura não se desloca de uma collunha.
  return ftxui::vbox({std::move(arte) | ftxui::hcenter, std::move(baixo)}) |
         ftxui::bgcolor(ftxui::Color::RGB(fundo.r, fundo.g, fundo.b)) |
         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(largura));
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
