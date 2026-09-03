// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO EXEMPLO DA FITA DA LOUSA — exemplos/fita_lousa.cpp
// ══════════════════════════════════════════════════════════════════════════
// Põe uma imagem no rectangulo pedido, espera, e tira-a. É a prova do OLHO da
// lousa sem se abrir o tocador: o que se vê aqui é o que o painel ha de
// mostrar. Escreve uma regua de collunhas e linhas por baixo, para que a
// posição se confira contando, e não por impressão.
//
// DOMÍNIO ......... um arquivo, o canto em `COLLUNHAxLINHA`, o rectangulo em
//                   `LARGURAxALTURA` de célullas, e os segundos de espera.
// CONTRA-DOMÍNIO .. a imagem na tela por esse tempo, e o status zero; ou nada
//                   na tela, queixa no erro, e o status dous.
// INVARIANTE ...... a lousa desfaz-se ao sahir, por qualquer das portas, e
//                   fantasma algum fica na tela.
// Q.E.D. .......... havendo regua debaixo da imagem, a prova do olho conta
//                   célullas em vez de as estimar.
// ══════════════════════════════════════════════════════════════════════════
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>

#include "nucleo/lousa.hpp"

namespace nu = mysong::nucleo;

namespace {

// par_de — o `AxB`, e sómente elle. Zero admitte-se, ao contrario do que faz o
// fita_capa: o canto do terminal É a collunha zero, e recusál-o tiraria da
// prova justamente a posição mais fácil de conferir.
bool par_de(std::string_view texto, long* um, long* outro) {
  const std::size_t cruz = texto.find('x');
  if (cruz == std::string_view::npos) return false;
  const std::string esquerda(texto.substr(0, cruz));
  const std::string direita(texto.substr(cruz + 1));
  char* resto = nullptr;
  *um = std::strtol(esquerda.c_str(), &resto, 10);
  if (resto == nullptr || *resto != '\0' || *um < 0) return false;
  *outro = std::strtol(direita.c_str(), &resto, 10);
  return resto != nullptr && *resto == '\0' && *outro >= 0;
}

}  // namespace

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
