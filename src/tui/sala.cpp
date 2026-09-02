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

#include "tui/sala.hpp"

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

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
