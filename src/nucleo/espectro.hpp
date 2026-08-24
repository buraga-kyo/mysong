// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ESPECTRO — src/nucleo/espectro.hpp
// ══════════════════════════════════════════════════════════════════════════
// A MATHEMATICA, e nada mais: recebe amostras, devolve bandas. Não sabe de
// PipeWire, não sabe de mpv, não abre linha de execução propria. É de
// proposito, e é a decisão que faz esta tarefa provavel: a colheita fala com o
// mundo e não se prova, porque não ha PipeWire determinístico; a transformação é
// CONTA, e conta se prova com sinal synthetico em machina surda.
//
// DOMÍNIO ......... amostras flotantes ENTRELAÇADAS, na taxa e no numero de
//                   canaes que o formato confirmado disser, em blocos de
//                   tamanho QUALQUER, inclusive zero e maiores que a janela: o
//                   quantum d'esta machina vae de 32 a 2048, e muda em voo.
// CONTRA-DOMÍNIO .. QUANTAS_BANDAS magnitudes em [0,1], suavizadas, promptas
//                   para barra de terminal.
// INVARIANTE ...... silencio absoluto dá zero EXACTO, e não erro. As bandas
//                   nunca sahem de [0,1], nunca sahem NaN e nunca sahem em
//                   numero differente de QUANTAS_BANDAS. As bordas se calculam
//                   da taxa CONFIRMADA, jamais de 48000 chumbado: placa de
//                   44100 muda a largura da raia, e as bordas com ella.
// Q.E.D. .......... um seno de frequencia conhecida acende a banda que o
//                   contém, e não as que estão longe d'ella. É prova exacta, e
//                   não depende de machina nem de placa de som. E a fftw3 não
//                   entra por este cabeçalho, que declara o plano adiante, do
//                   mesmo modo que o motor declara o punho da libmpv.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <vector>

#include "nucleo/analisador.hpp"  // de onde vem QUANTAS_BANDAS, que é do CONTRACTO

// O plano da fftw3, declarado adiante e não incluido: assim fftw3.h não entra
// por este cabeçalho em unidade alguma que não precise d'ella, e a bateria da
// mathematica compila sem os directorios de inclusão da fftw.
extern "C" {
struct fftwf_plan_s;
}

namespace mysong::nucleo {

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
