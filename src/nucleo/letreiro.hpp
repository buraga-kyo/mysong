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
  std::string familia{FAMILIA_DA_MARCA};
  std::string tinta;
  std::string fundo;
  std::size_t cellulas = 0;
  int corpo = CORPO_DA_MARCA;
};

// argumentos_do_letreiro — o que se corre, pelo precedente do
// `argumentos_do_chafa`: a linha á parte e PURA, para que a bateria afira o
// que se HA DE correr sem correr programa algum. O `-q` cala a janella que o
// pango-view abriria, e o `--margin` leva DOUS numeros, que é como elle diz o
// vertical e o horizontal: a folga vae em cima e em baixo, e nunca aos lados,
// que largura a mais deslocaria a palavra dentro da caixa.
std::vector<std::string> argumentos_do_letreiro(
    const Pedido& pedido, std::size_t margem,
    const std::filesystem::path& sahida);

// margem_da_chapa — a folga, em pixeis, que casa a proporção da chapa com a
// da CAIXA. Sem ella o Überzug++, que encolhe guardando a proporção, deixaria
// a chapa mais chata que a linha: a palavra em XIROD é larga, e a caixa da
// aba é uma cella de altura, d'onde a largura manda na conta e a chapa
// pararia a meia altura, com o mono de baixo a espreitar por fóra.
//
// PURA, e sem parametro de omissão pela razão do `rectangulo_da_capa`: a
// cella entra por parametro, e trocando elle o corpo da fonte é UM numero.
std::size_t margem_da_chapa(Medida crua, std::size_t cellulas, Medida cellula);

// chave_do_letreiro — o nome do arquivo em cache, e a somma de TODO o pedido:
// texto, tintas, corpo e célullas. As célullas entram porque a proporção sahe
// d'ellas, e chapa da mesma palavra em caixa mais larga é outra imagem.
std::string chave_do_letreiro(const Pedido& pedido);

// caminho_da_chapa_em_cache — `$XDG_CACHE_HOME/mysong/letreiro/<chave>.png`,
// ao lado das capas e pela mesma razão: o Überzug++ lê DISCO, e não memoria.
// Vazio sem XDG_CACHE_HOME e sem HOME, que ahi cache não ha.
std::filesystem::path caminho_da_chapa_em_cache(const Pedido& pedido);

// parecer_do_letreiro — a decisão, PURA pelo molde do `parecer_da_lousa`: o
// mundo entra por dous bools, para que o caminho da recusa se observe n'esta
// machina, que tem os dous. O ajuste da LOUSA manda aqui, e chave propria não
// se abriu: chapa sem lousa não tem onde se pôr, e duas alavancas para a mesma
// cousa dariam ao operador dous logares por onde desligar uma só.
Parecer parecer_do_letreiro(ModoDaLousa modo, bool ha_pango, bool ha_familia);

// As duas perguntas ao MUNDO, á parte do parecer pela razão do `ha_display`.
bool ha_pango_view();
bool ha_familia_da_marca();

// texto_do_letreiro — a linha do --sonda, pura pelo precedente do
// `texto_da_lousa`: escape algum sahe d'aqui.
std::string texto_do_letreiro(const Parecer& parecer);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
