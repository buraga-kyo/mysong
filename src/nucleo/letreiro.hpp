// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO LETREIRO — src/nucleo/letreiro.hpp
// ══════════════════════════════════════════════════════════════════════════
// O LETREIRO é a palavra de MARCA rasterizada em imagem. Terminal algum troca
// de fonte por cella, e a XIROD do RADICAL-OS não é fonte de terminal: quem a
// desenha é o `pango-view`, e quem a põe por cima da cella é a lousa. Sómente
// a MARCA vae por aqui; o nome da faixa, o tempo e o volume são DADO, e dado
// fica em mono.
//
// DOMÍNIO ......... um texto, o par de tintas, e a largura em CÉLULLAS.
// CONTRA-DOMÍNIO .. o caminho de um PNG em cache; ou vazio, e ahi a cella fica
//                   com o mono que já pintava por baixo.
// INVARIANTE ...... a segunda chamada com o mesmo pedido não corre programa
//                   algum; e a chapa nasce com a PROPORÇÃO da caixa.
// Q.E.D. .......... sendo a linha de commando e a chave do cache funcções
//                   puras, a bateria afere o que se HA DE correr sem correr.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "nucleo/capa.hpp"   // Medida: a chapa mede-se como a capa se mede
#include "nucleo/lousa.hpp"  // Parecer e ModoDaLousa: sem lousa não ha chapa

namespace mysong::nucleo {

// A FAMILIA da marca, e o CORPO. O corpo MEDIU-SE: a vinte e dous pontos a
// palavra sahe com trinta e sete pixeis de altura, quase o dobro dos vinte da
// cella, e é d'essa folga que a reducção do Überzug++ tira o traço limpo.
inline constexpr std::string_view FAMILIA_DA_MARCA = "Xirod";
inline constexpr int CORPO_DA_MARCA = 22;

// Um PEDIDO de chapa. As tintas vão em hexadecimal por o `pango-view` as
// querer assim, e vêm SEMPRE de `tui::tokens`: côr crua não entra n'esta obra.
struct Pedido {
  std::string texto;
  std::string tinta;
  std::string fundo;
  std::size_t cellulas = 0;
  int corpo = CORPO_DA_MARCA;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
