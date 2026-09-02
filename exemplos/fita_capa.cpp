// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO EXEMPLO DA FITA DA CAPA — exemplos/fita_capa.cpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta a capa no terminal, com os MESMOS argumentos que o painel usa, para se
// comparar o render de hontem com o de hoje sem abrir o tocador. Prova de
// beleza não é, que beleza sómente o olho julga; é o instrumento com que elle
// a julga, e cuja sahida se confere com o grep.
//
// O terceiro argumento força os sextantes, ou tira-os; sem elle vale a regra da
// Casa, que é perguntar á fonte. Assim se comparam os dous no mesmo terminal.
//
// DOMÍNIO ......... um arquivo, a geometria `LARGURAxALTURA`, e quando muito a
//                   palavra que força o sextante.
// CONTRA-DOMÍNIO .. as linhas do render na sahida padrão e o status zero; ou
//                   nada na sahida, queixa no erro, e o status dous.
// INVARIANTE ...... a tinta sahe IMMEDIATAMENTE antes do glifo, e cada linha
//                   remata em repouso.
// Q.E.D. .......... redigida a sahida a um arquivo, os bytes dizem que glifos
//                   sahiram e com que tinta; a fresta é materia do olho.
// ══════════════════════════════════════════════════════════════════════════
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "nucleo/capa.hpp"
#include "tui/tokens.hpp"

namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;

namespace {

// medida_de — o `LARGURAxALTURA`, e sómente elle. Numero mal escripto, sobra
// depois d'elle, ou zero, recusam-se aqui: a queixa nossa nomeia o argumento, e
// a do chafa («Size must be at least 1x1») não.
bool medida_de(std::string_view texto, std::size_t* collunas,
               std::size_t* linhas) {
  const std::size_t cruz = texto.find('x');
  if (cruz == std::string_view::npos) return false;
  char* resto = nullptr;
  const std::string largo(texto.substr(0, cruz)), alto(texto.substr(cruz + 1));
  const long l = std::strtol(largo.c_str(), &resto, 10);
  if (resto == nullptr || *resto != '\0' || l < 1) return false;
  const long a = std::strtol(alto.c_str(), &resto, 10);
  if (resto == nullptr || *resto != '\0' || a < 1) return false;
  *collunas = static_cast<std::size_t>(l);
  *linhas = static_cast<std::size_t>(a);
  return true;
}

// tela_de — as corridas em SGR crú. A tinta sahe IMMEDIATAMENTE antes do glifo
// que veste, sem repouso pelo meio, e o repouso vem sómente ao fim da linha: é
// o arranjo da fita do espectro, e é elle que fecha a emenda entre corridas
// vizinhas de côres differentes. Funcção PURA: não escreve byte algum.
std::string tela_de(const nu::CapaPintada& capa) {
  const auto veste = [](int papel, int r, int g, int b) {
    return tk::sgr(papel, tk::Triade{static_cast<unsigned char>(r),
                                     static_cast<unsigned char>(g),
                                     static_cast<unsigned char>(b)});
  };
  std::string tela;
  for (const std::vector<nu::Corrida>& linha : capa.linhas) {
    for (const nu::Corrida& corrida : linha) {
      if (corrida.r_frente >= 0)
        tela += veste(38, corrida.r_frente, corrida.g_frente, corrida.b_frente);
      if (corrida.r_fundo >= 0)
        tela += veste(48, corrida.r_fundo, corrida.g_fundo, corrida.b_fundo);
      tela += corrida.texto;
    }
    tela += tk::repouso;
    tela += '\n';
  }
  return tela;
}

}  // namespace
