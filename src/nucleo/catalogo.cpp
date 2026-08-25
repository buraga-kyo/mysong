// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO CATALOGO — src/nucleo/catalogo.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. O recorte do JSON vem de api/jsonzinho.hpp, e não se repete
// aqui: a letra da issue #14 já usa o mesmo.
//
// DOMÍNIO ......... a URL, e o corpo da pagina de embutir.
// CONTRA-DOMÍNIO .. o catalogo.
// INVARIANTE ...... faixa sem titulo não sahe, e a ordem que se põe é a da LISTA e
//                   não a das que sobreviveram: a faixa tres continua a ser a tres
//                   ainda que a dous se tenha recusado.
// Q.E.D. .......... o `trackList` acha-se por nome em qualquer fundo do embrulho,
//                   donde a leitura sobrevive a o Spotify mudar de quantos niveis
//                   de embrulho o rodeia, que é cousa que elle muda.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/catalogo.hpp"

#include <curl/curl.h>

#include <cstddef>
#include <utility>

#include "api/jsonzinho.hpp"

namespace mysong::nucleo {
namespace {

std::size_t recolhe(char* pedaco, std::size_t tamanho, std::size_t quantos,
                    void* fora) {
  const std::size_t bytes = tamanho * quantos;
  static_cast<std::string*>(fora)->append(pedaco, bytes);
  return bytes;
}

// O identificador do Spotify tem vinte e dous caracteres de base sessenta e dous.
// Não se afere o comprimento: elle é convenção d'elles e pode mudar. Afere-se o
// ALPHABETO, que é o que separa identificador de pedaço de URL.
bool letra_de_id(char octeto) {
  return (octeto >= 'a' && octeto <= 'z') || (octeto >= 'A' && octeto <= 'Z') ||
         (octeto >= '0' && octeto <= '9');
}

}  // namespace

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
