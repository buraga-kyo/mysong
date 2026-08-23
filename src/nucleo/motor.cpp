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

// Lê um dobro do mpv. Propriedade que ainda não existe devolve zero, e não
// erro: quem nada toca não tem posição, e zero é o retracto d'esse nada.
double le_dobro(::mpv_handle* punho, const char* nome) {
  double valor = 0.0;
  if (mpv_get_property(punho, nome, MPV_FORMAT_DOUBLE, &valor) < 0) return 0.0;
  return valor;
}

}  // namespace

MotorMpv::MotorMpv(::mpv_handle* punho) noexcept : punho_(punho) {}

// O cedente sahe com punho nullo, e é d'ahi que o destructor nunca desfaz duas
// vezes o mesmo punho.
MotorMpv::MotorMpv(MotorMpv&& outro) noexcept
    : Motor(),
      punho_(outro.punho_),
      estado_(outro.estado_),
      posicao_(outro.posicao_),
      duracao_(outro.duracao_) {
  outro.punho_ = nullptr;
}

MotorMpv::~MotorMpv() {
  if (punho_ != nullptr) mpv_terminate_destroy(punho_);
}

// A FABRICA. Unico caminho para um MotorMpv existir; quem falha não tem
// objecto, e não um objecto a que se deva perguntar se serve.
std::optional<MotorMpv> MotorMpv::abrir(std::string* razao) {
  ::mpv_handle* punho = mpv_create();
  if (punho == nullptr) {
    if (razao) *razao = "mpv_create não deu punho algum";
    return std::nullopt;
  }

  // ANTES de mpv_initialize, sem o que as opções se ignoram em silencio.
  // «ao=pipewire» é load-bearing: veja a segunda invariante do tractado.
  if (!assenta(punho, "ao", "pipewire", razao) ||
      !assenta(punho, "video", "no", razao) ||
      !assenta(punho, "idle", "yes", razao)) {
    mpv_terminate_destroy(punho);
    return std::nullopt;
  }

  const int codigo = mpv_initialize(punho);
  if (codigo < 0) {
    if (razao) {
      *razao = std::string("mpv_initialize: ") + mpv_error_string(codigo);
    }
    mpv_terminate_destroy(punho);
    return std::nullopt;
  }

  mpv_observe_property(punho, 0, "time-pos", MPV_FORMAT_DOUBLE);
  mpv_observe_property(punho, 0, "duration", MPV_FORMAT_DOUBLE);
  return MotorMpv(punho);
}

unsigned long MotorMpv::versao_da_interface() noexcept {
  return mpv_client_api_version();
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
