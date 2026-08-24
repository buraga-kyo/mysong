// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA TABOA DA LIBMPV — src/nucleo/libmpv.hpp
// ══════════════════════════════════════════════════════════════════════════
// A libmpv NÃO se liga a binário algum d'esta obra: carrega-se por dlopen, e as
// suas funcções chamam-se por esta taboa de ponteiros. É o que faz valer a
// sonda dos requisitos: o processo SOBE sem a bibliotheca, e a sonda a apanha e
// nomeia. Ligada, o carregador mataria o processo antes do main.
// DOMÍNIO ......... o soname da bibliotheca, tomado da taboa da sonda.
// CONTRA-DOMÍNIO .. ou a taboa com as treze funcções atadas, ou ausencia com
//                   razão que NOMEIA a falta: soname, symbolo, ou versão.
// INVARIANTE ...... taboa devolvida tem os treze campos não nullos. Campo por
//                   atar reprova o carregamento INTEIRO, e nunca dá taboa meia:
//                   doze de treze subiria, tocaria, e morreria no primeiro uso
//                   do que faltou, modo de falhar que esta Casa nega.
// INVARIANTE 2 .... a bibliotheca nunca se solta: punho vivo com ella
//                   descarregada derruba o processo.
// Q.E.D. .......... as assignaturas tiram-se do cabeçalho por decltype, e
//                   nenhuma se copia á mão; donde não podem divergir d'elle.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <mpv/client.h>
#include <string>
#include <string_view>

namespace mysong::nucleo {

// A LISTA das treze. Funcção nova é NOME novo aqui, e nada mais: a taboa e o
// carregador crescem por esta macro, e não por linha á mão em dous logares.
#define MYSONG_LIBMPV_FUNCCOES(FAZ)                                    \
  FAZ(mpv_create) FAZ(mpv_initialize) FAZ(mpv_terminate_destroy)        \
  FAZ(mpv_set_option_string) FAZ(mpv_command) FAZ(mpv_wait_event)       \
  FAZ(mpv_observe_property) FAZ(mpv_get_property)                       \
  FAZ(mpv_get_property_string) FAZ(mpv_set_property) FAZ(mpv_free)      \
  FAZ(mpv_error_string) FAZ(mpv_client_api_version)

// A TABOA: um campo por funcção, com o typo tirado do proprio cabeçalho.
struct TaboaDaLibmpv {
#define MYSONG_LIBMPV_CAMPO(nome) decltype(&::nome) nome = nullptr;
  MYSONG_LIBMPV_FUNCCOES(MYSONG_LIBMPV_CAMPO)
#undef MYSONG_LIBMPV_CAMPO
};

// UMA tentativa de atar a taboa ao soname dado, sem guardar nada: é a forma que
// a prova exercita, pois a de libmpv() faz-se uma vez por processo e não se
// repete com soname diverso.
bool carregar_taboa(std::string_view soname, TaboaDaLibmpv* taboa,
                    std::string* razao);

// A taboa, aberta no primeiro pedido e guardada para o processo todo. Nulo é
// ausencia, e então *razao recebe o que faltou, pelo nome.
const TaboaDaLibmpv* libmpv(std::string* razao = nullptr);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
