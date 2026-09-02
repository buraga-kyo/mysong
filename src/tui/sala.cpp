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

#include "tui/sala.hpp"
#include "tui/tabella.hpp"
#include "tui/tokens.hpp"

namespace mysong::tui {
namespace {
// Os numeros da sala. LIMIAR: 96 collunhas UTEIS, que são as cem da tela do
// operador menos as quatro da orla; abaixo d'ellas o painel roubaria da
// tabella, que é onde se navega, para mostrar arte, que é o que enfeita.
constexpr std::size_t kLimiarDoPainel = 96, kPainelMinimo = 24;
// O painel tem quatro linhas fixas (o titulo e as tres da ficha) e o espectro
// não desce de oito: d'onde abaixo de doze linhas de corpo elle não se pinta.
constexpr std::size_t kFixoDoPainel = 4, kEspectroMinimo = 8, kCapaMinima = 4;
// O meio não desce de quarenta collunhas, e o cabeçalho (capa pequena e o
// separador) cede o logar á tabella quando ella ficaria com menos de tres.
constexpr std::size_t kMeioMinimo = 40, kCabecalho = 6, kTabellaMinima = 3;

// kMarcadorLinhas — a área do marcador. Seis, e não o tecto: a capa de 16 por
// 9 sahe em cerca d'onze linhas n'um painel de 39, e moldura vazia de vinte
// diria «não ha capa» mais alto do que o painel diz a musica.
constexpr std::size_t kMarcadorLinhas = 6;
// A capa pequena do cabeçalho: dez por cinco, que a cella é de dous por um.
constexpr std::size_t kCapaPequena = 10, kCapaPequenaLinhas = 5;

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

std::string nome_da_colleccao(Secao secao,
                              const std::vector<std::string>& trilha,
                              const std::string& nome_do_catalogo) {
  // Dentro de alguma cousa, o nome é o do degrau em que se entrou: o artista em
  // ÁLBUNS, o album em FAIXAS, a lista em NoRol. Fóra, o rotulo da secção.
  const bool dentro = !trilha.empty();
  switch (secao) {
    case Secao::Artistas: return "ARTISTAS";
    case Secao::Albuns: return dentro ? trilha.back() : "ÁLBUNS";
    case Secao::Faixas: return dentro ? trilha.back() : "FAIXAS";
    case Secao::Busca: return "MINHAS MÚSICAS";
    case Secao::Rede: return "REDE";
    case Secao::Rois: return "LISTAS";
    case Secao::NoRol: return dentro ? trilha.back() : "LISTAS";
    case Secao::Lista: break;
  }
  return nome_do_catalogo.empty() ? "SPOTIFY" : nome_do_catalogo;
}

Geometria geometria_da_sala(std::size_t largura, std::size_t altura,
                            std::size_t collunhas_da_barra) {
  Geometria geo;
  const std::size_t util =
      largura > collunhas_da_barra ? largura - collunhas_da_barra : 1;
  if (largura >= kLimiarDoPainel && altura >= kFixoDoPainel + kEspectroMinimo &&
      util > kMeioMinimo + kPainelMinimo) {
    geo.painel = std::max<std::size_t>(kPainelMinimo, largura / 4);
    geo.painel = std::min(geo.painel, util - kMeioMinimo - 1);
    geo.livre = altura > kFixoDoPainel ? altura - kFixoDoPainel : 0;
    // Tecto: o que se PINTA é o que o chafa devolver, guardando a proporção.
    if (geo.livre >= kEspectroMinimo + kCapaMinima)
      geo.capa = std::min(geo.painel / 2 + 1, geo.livre - kEspectroMinimo);
  }
  geo.meio = geo.painel == 0 ? util : util - geo.painel - 1;
  geo.cabecalho = altura >= kCabecalho + kTabellaMinima ? kCabecalho : 0;
  geo.tabella = altura > geo.cabecalho ? altura - geo.cabecalho : 1;
  return geo;
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
  if (ficha.titulo.empty())
    return ftxui::vbox({pinta("(nada toca)", tokens::text_faint),
                        ftxui::text(""), ftxui::text("")});
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
  if (largura == 0 || linhas == 0) return ftxui::text("");
  if (capa.achada) return elemento_da_capa(capa, largura, linhas);
  if (largura <= 2 || linhas <= 2) return ftxui::text("");
  return elemento_da_capa(capa, largura - 2, linhas - 2);
}

ftxui::Element elemento_do_cabecalho(const Colleccao& colleccao,
                                     const nucleo::CapaPintada& capa,
                                     std::size_t largura) {
  if (largura == 0) return ftxui::text("");
  const bool repete = colleccao.repeticao != nucleo::Repeticao::Nenhuma;
  const bool uma = colleccao.repeticao == nucleo::Repeticao::Uma;
  const char* qual = !repete ? "NÃO" : uma ? "UMA" : "TODAS";
  std::string risca;
  for (std::size_t c = 0; c < largura; ++c) risca += "\u2500";
  return ftxui::vbox(
      {ftxui::hbox({elemento_da_arte(capa, kCapaPequena, kCapaPequenaLinhas),
                    ftxui::text("  "),
                    ftxui::vbox({
                        ftxui::text(""),
                        pinta(colleccao.nome, tokens::text_heading) |
                            ftxui::bold,
                        ftxui::text(""),
                        ftxui::hbox(
                            {pinta(texto_da_conta(colleccao.quantas,
                                                  colleccao.especie,
                                                  colleccao.duracao) + "  ",
                                   tokens::text_body),
                             chip(" \u21c4 EMBARALHAR ", colleccao.embaralhado),
                             ftxui::text(" "),
                             chip(std::string(" \u21bb REPETIR: ") + qual + " ",
                                  repete)}),
                        ftxui::text(""),
                    })}) |
           ftxui::size(ftxui::HEIGHT, ftxui::EQUAL,
                       static_cast<int>(kCapaPequenaLinhas)),
       pinta(risca, tokens::line_dim)});
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
