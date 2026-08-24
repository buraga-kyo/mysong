// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO CARREGAMENTO DA LIBMPV — src/nucleo/libmpv.cpp
// ══════════════════════════════════════════════════════════════════════════
// DOMÍNIO ......... o soname da bibliotheca, e o carregador dinamico.
// CONTRA-DOMÍNIO .. a taboa atada, ou nulo com razão que NOMEIA a falta.
// INVARIANTE ...... symbolo por atar reprova o carregamento INTEIRO: a taboa
//                   sahe cheia ou não sahe, e nunca meia.
// Q.E.D. .......... uma tentativa por processo, e o seu resultado guardado;
//                   donde perguntar mil vezes custa o mesmo que perguntar uma.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/libmpv.hpp"

#include <dlfcn.h>

namespace mysong::nucleo {

// Abre-se no primeiro pedido, por estatico local que o C++ serializa: o tocador
// e o socket podem pedir de fios diversos. E NÃO se solta: dlclose com punho do
// mpv vivo derruba o processo, e por isso a bibliotheca fica pela vida d'elle.
const TaboaDaLibmpv* libmpv(std::string* razao) {
  static std::string queixa;
  static TaboaDaLibmpv taboa;
  static const bool atada = [] {
    void* biblio = dlopen("libmpv.so.2", RTLD_LAZY | RTLD_LOCAL);
    if (biblio == nullptr) {
      const char* const erro = dlerror();
      queixa = std::string("não achei a libmpv.so.2: ") +
               (erro != nullptr ? erro : "sem razão dita");
      return false;
    }
#define MYSONG_LIBMPV_ATA(nome)                                             \
  taboa.nome = reinterpret_cast<decltype(&::nome)>(dlsym(biblio, #nome));   \
  if (taboa.nome == nullptr) {                                              \
    queixa = "a libmpv não traz o symbolo " #nome;                          \
    return false;                                                           \
  }
    MYSONG_LIBMPV_FUNCCOES(MYSONG_LIBMPV_ATA)
#undef MYSONG_LIBMPV_ATA
    return true;
  }();
  if (!atada && razao) *razao = queixa;
  return atada ? &taboa : nullptr;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
