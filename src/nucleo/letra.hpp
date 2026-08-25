// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LETRA — src/nucleo/letra.hpp
// ══════════════════════════════════════════════════════════════════════════
// Busca a letra no LRCLIB e grava-a em `.lrc` ao lado do audio. A busca acontece
// UMA vez, no momento do download; escutar NUNCA toca a rede. É decisão do
// operador, e ella tem consequencia de desenho: o tocador lê arquivo, e não
// serviço, donde tocar sem internet é o caso ordinario e não a excepção.
//
// DOMÍNIO ......... o que se sabe da faixa (artista, titulo, album, duração), e
//                   o corpo que o LRCLIB devolveu.
// CONTRA-DOMÍNIO .. um arquivo `.lrc` ao lado do audio, ou nada, sem erro.
// INVARIANTE ...... letra ausente NÃO é falha. A maior parte do acervo de
//                   qualquer um não tem letra sincronizada, e tratar isso como
//                   erro faria o download parecer roto quando está inteiro.
// Q.E.D. .......... sendo puras a montagem da URL, o recorte do primeiro objecto
//                   e a leitura da resposta, a bateria afere-as sobre corpos
//                   escriptos á mão, sem tocar a rede.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace mysong::nucleo {

// A LETRA como o LRCLIB a dá: a sincronizada, com os carimbos de tempo, e a
// plana. Vazias as duas quer dizer que não ha letra, e não que houve erro.
struct Letra {
  std::string sincronizada;
  std::string plana;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
