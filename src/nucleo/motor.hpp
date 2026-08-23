// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MOTOR — src/nucleo/motor.hpp
// ══════════════════════════════════════════════════════════════════════════
// Declara o MOTOR: a potencia que faz sahir som de um arquivo, e as ordens a
// que ella responde. Não é o tocador, que governa a fila; é só a potencia.
// Declara-se ABSTRACTA de proposito, para que a bateria de provas ponha em seu
// logar um dublê que não abre placa de som alguma.
//
// DOMÍNIO ......... um caminho de arquivo no systema, e ordens de operador:
//                   tocar, pausar, retomar, buscar, volume.
// CONTRA-DOMÍNIO .. som na saída de áudio, e tres grandezas legíveis a
//                   qualquer instante: posição, duração e estado.
// INVARIANTE ...... o motor NÃO toca o volume do systema. O volume d'aqui é o
//                   do proprio motor, e o contracto declarado de vol.sh fica
//                   intacto. Nem governa fila: uma faixa de cada vez, e a
//                   ordem é de quem chama, nunca do motor.
// Q.E.D. .......... sendo a interface abstracta, a prova da fila e das
//                   transições corre em máquina surda; e sendo o volume o do
//                   motor e não o do systema, provar que o systema não mudou
//                   reduz-se a medi-lo antes e depois.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace mysong::nucleo {

// O estado do motor. Tres, e não mais: quem não toca nem pausa, está parado.
enum class Estado { Parado, Tocando, Pausado };

// O nome do estado, para relatorio de prova e para olho humano.
std::string_view nome_do_estado(Estado estado) noexcept;

// O que se annuncia a quem escuta. Quatro avisos, e nenhum d'elles carrega
// quem os ouve: o nucleo emitte ao vento, e quem quiser recolhe.
enum class Aviso { FaixaMudou, EstadoMudou, PosicaoAndou, FalhouAoTocar };

// O pregão: o aviso, e o retracto do mundo no instante em que se deu. Vae por
// valor e completo, de sorte que o ouvinte nada precise interrogar de volta —
// interrogar de volta seria o ouvinte conhecer o nucleo, e não sómente o
// nucleo ignorar o ouvinte.
struct Evento {
  Aviso aviso = Aviso::EstadoMudou;
  Estado estado = Estado::Parado;
  std::string faixa;        // vazia quando nenhuma faixa esta em curso
  double posicao = 0.0;     // em segundos, contados do inicio da faixa
  std::string razao;        // preenchida sómente em FalhouAoTocar
};

// Quem escuta. Zero ouvintes é caso legitimo e não erro: a emissão ao vento,
// sem ninguem que a recolha, é o estado normal da bateria de provas.
using Ouvinte = std::function<void(const Evento&)>;

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
