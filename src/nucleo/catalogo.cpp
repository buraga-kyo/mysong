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

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
