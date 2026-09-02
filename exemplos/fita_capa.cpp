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

#include "nucleo/capa.hpp"

namespace nu = mysong::nucleo;

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

}  // namespace
