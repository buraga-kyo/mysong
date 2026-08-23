// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MOTOR MPV, LAVRA — src/nucleo/motor.cpp
// ══════════════════════════════════════════════════════════════════════════
// A UNICA unidade de traducção do reino que inclue mpv/client.h. Todo o resto
// do nucleo fala com o Motor abstracto, e por isso todo o resto se prova sem
// placa de som.
//
// DOMÍNIO ......... um punho da libmpv, e ordens já aparadas ou por aparar.
// CONTRA-DOMÍNIO .. som na saída do PipeWire, e as grandezas do relogio.
// INVARIANTE ...... o punho pertence a UM objecto só. A cópia esta supprimida,
//                   o move deixa o cedente com punho nullo, e o destructor
//                   chama mpv_terminate_destroy uma vez e uma só. Não ha
//                   caminho, nem por excepção, que o duplique ou o perca.
// INVARIANTE 2 .... ao=pipewire assenta-se ANTES de mpv_initialize, sem o que
//                   a opção se ignora. É LOAD-BEARING: a issue #5 ha de achar
//                   o nó do mpv no grafo do PipeWire por nome previsivel, e
//                   trocar esta saída quebra a issue seguinte.
// Q.E.D. .......... a fila NÃO desce ao mpv: toda ordem de tocar vae com
//                   «replace», de sorte que a playlist do mpv nunca passa de
//                   uma entrada, e a ordem das faixas fica inteira com nós.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/motor.hpp"

#include <mpv/client.h>

#include <string_view>

namespace mysong::nucleo {
namespace {

// Assenta uma opção ANTES de mpv_initialize, e diz porque falhou se falhar.
bool assenta(::mpv_handle* punho, const char* nome, const char* valor,
             std::string* razao) {
  const int codigo = mpv_set_option_string(punho, nome, valor);
  if (codigo >= 0) return true;
  if (razao) {
    *razao = std::string("não pude assentar ") + nome + '=' + valor + ": " +
             mpv_error_string(codigo);
  }
  return false;
}

}  // namespace
}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
