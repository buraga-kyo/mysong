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

#include "nucleo/sonda.hpp"

namespace mysong::nucleo {
namespace {

// O soname e a chave vêm da taboa da SONDA, e não de cadeia repetida aqui:
// divergindo as duas, a sonda daria por presente o que o motor daria por
// ausente, e a tela das faltas contradiria a recusa do motor.
const Requisito* requisito_da_libmpv() {
  for (const Requisito& requisito : requisitos())
    if (requisito.chave == "libmpv") return &requisito;
  return nullptr;
}

// A interface afere-se pelo MAIOR, e nunca pelo menor: a libmpv promette
// compatibilidade para deante no menor, e recusar por elle seria mais severo do
// que a bibliotheca pede. Maior diverso é outra interface, e não a nossa.
bool maior_diverso(unsigned long achada) {
  return (achada >> 16) != (MPV_CLIENT_API_VERSION >> 16);
}

// Ata os treze, e para no PRIMEIRO que falte, nomeando-o.
bool atar(void* biblio, TaboaDaLibmpv* taboa, std::string* razao) {
#define MYSONG_LIBMPV_ATA(nome)                                            \
  taboa->nome = reinterpret_cast<decltype(&::nome)>(dlsym(biblio, #nome)); \
  if (taboa->nome == nullptr) {                                            \
    if (razao) *razao = "a libmpv não traz o symbolo " #nome;               \
    return false;                                                          \
  }
  MYSONG_LIBMPV_FUNCCOES(MYSONG_LIBMPV_ATA)
#undef MYSONG_LIBMPV_ATA
  return true;
}

}  // namespace

// O dlopen, o laço dos treze e a aferição, sem memoria alguma. A bibliotheca NÃO
// se solta: dlclose com punho do mpv vivo derruba o processo.
bool carregar_taboa(std::string_view soname, TaboaDaLibmpv* taboa,
                    std::string* razao) {
  const std::string nome(soname);
  void* biblio = dlopen(nome.c_str(), RTLD_LAZY | RTLD_LOCAL);
  if (biblio == nullptr) {
    const char* const erro = dlerror();
    if (razao) {
      *razao = "não achei a " + nome + ": " + (erro ? erro : "sem razão");
    }
    return false;
  }
  if (!atar(biblio, taboa, razao)) return false;
  const unsigned long achada = taboa->mpv_client_api_version();
  if (maior_diverso(achada)) {
    if (razao) {
      *razao = "a libmpv é de interface " + std::to_string(achada >> 16) +
               ", e a obra compilou-se para a " +
               std::to_string(MPV_CLIENT_API_VERSION >> 16);
    }
    return false;
  }
  return true;
}

// Abre-se no primeiro pedido, por estatico local que o C++ serializa: o tocador
// e o socket podem pedir de fios diversos.
const TaboaDaLibmpv* libmpv(std::string* razao) {
  static std::string queixa;
  static TaboaDaLibmpv taboa;
  static const bool atada = [] {
    const Requisito* const requisito = requisito_da_libmpv();
    if (requisito == nullptr) {
      queixa = "a taboa da sonda não declara requisito de chave libmpv";
      return false;
    }
    if (nomeado_na_forcagem(requisito->chave)) {
      queixa = "MYSONG_SONDA_FORCA nomeia " + std::string(requisito->chave) +
               ": finge-se ausente a " + std::string(requisito->alvo);
      return false;
    }
    return carregar_taboa(requisito->alvo, &taboa, &queixa);
  }();
  if (!atada && razao) *razao = queixa;
  return atada ? &taboa : nullptr;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
