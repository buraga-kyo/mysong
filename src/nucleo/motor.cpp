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

#include "nucleo/libmpv.hpp"

#include <string_view>

namespace mysong::nucleo {
namespace {

// A taboa, já atada. Nunca é nulla aqui: abrir() é a UNICA porta para um
// MotorMpv existir, e recusa antes de tudo quando a taboa não abre; donde toda
// linha d'este arquivo que chame por aqui corre depois d'aquella recusa.
const TaboaDaLibmpv& mpv() { return *libmpv(); }

// Assenta uma opção ANTES de mpv_initialize, e diz porque falhou se falhar.
bool assenta(::mpv_handle* punho, const char* nome, const char* valor,
             std::string* razao) {
  const int codigo = mpv().mpv_set_option_string(punho, nome, valor);
  if (codigo >= 0) return true;
  if (razao) {
    *razao = std::string("não pude assentar ") + nome + '=' + valor + ": " +
             mpv().mpv_error_string(codigo);
  }
  return false;
}

// Lê um dobro do mpv. Propriedade que ainda não existe devolve zero, e não
// erro: quem nada toca não tem posição, e zero é o retracto d'esse nada.
double le_dobro(::mpv_handle* punho, const char* nome) {
  double valor = 0.0;
  if (mpv().mpv_get_property(punho, nome, MPV_FORMAT_DOUBLE, &valor) < 0)
    return 0.0;
  return valor;
}

// A CAMA do punho enquanto a fabrica o arma. Desfaz o que ainda não achou
// dono, por QUALQUER caminho de sahida, excepção inclusa; e solta-se quando o
// MotorMpv toma posse. É o que faz a segunda invariante do tractado ser verdade
// em vez de intenção: sem ella, uma excepção erguida entre o mpv_create e a
// recusa deixaria o punho aberto e ninguem a quem cobrar.
class Cama {
 public:
  explicit Cama(::mpv_handle* punho) noexcept : punho_(punho) {}
  ~Cama() {
    if (punho_ != nullptr) mpv().mpv_terminate_destroy(punho_);
  }
  Cama(const Cama&) = delete;
  Cama& operator=(const Cama&) = delete;

  ::mpv_handle* punho() const noexcept { return punho_; }

  ::mpv_handle* solta() noexcept {
    ::mpv_handle* cedido = punho_;
    punho_ = nullptr;
    return cedido;
  }

 private:
  ::mpv_handle* punho_;
};

// Pede aviso das duas grandezas do relogio. Devolve o codigo da primeira que
// recusar, e não zero engolido: sem estes avisos o motor nunca reportaria
// posição nem duração, e o relogio ficaria mudo EM SILENCIO, que é o modo de
// falhar que esta Casa não aceita.
int observa_relogio(::mpv_handle* punho) {
  const int pela_posicao =
      mpv().mpv_observe_property(punho, 0, "time-pos", MPV_FORMAT_DOUBLE);
  if (pela_posicao < 0) return pela_posicao;
  return mpv().mpv_observe_property(punho, 0, "duration", MPV_FORMAT_DOUBLE);
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
  if (punho_ != nullptr) mpv().mpv_terminate_destroy(punho_);
}

// A FABRICA. Unico caminho para um MotorMpv existir; quem falha não tem
// objecto, e não um objecto a que se deva perguntar se serve.
std::optional<MotorMpv> MotorMpv::abrir(std::string* razao) {
  // A taboa ANTES de tudo: sem ella não ha funcção que se chame, e a razão que
  // ella devolve é a que o operador ha de ler. É por aqui que a falta da
  // libmpv chega como recusa nomeada, e não como morte no carregador.
  if (libmpv(razao) == nullptr) return std::nullopt;

  // Da cama ao MotorMpv, o punho tem dono a todo instante: nenhum caminho de
  // sahida d'esta funcção o deixa aberto, e nenhum d'elles o desfaz duas vezes.
  Cama cama(mpv().mpv_create());
  if (cama.punho() == nullptr) {
    if (razao) *razao = "mpv_create não deu punho algum";
    return std::nullopt;
  }

  // ANTES de mpv_initialize, sem o que as opções se ignoram em silencio.
  // «ao=pipewire» é load-bearing: veja a segunda invariante do tractado.
  if (!assenta(cama.punho(), "ao", "pipewire", razao) ||
      !assenta(cama.punho(), "video", "no", razao) ||
      !assenta(cama.punho(), "idle", "yes", razao)) {
    return std::nullopt;
  }

  const int codigo = mpv().mpv_initialize(cama.punho());
  if (codigo < 0) {
    if (razao) {
      *razao = std::string("mpv_initialize: ") +
               mpv().mpv_error_string(codigo);
    }
    return std::nullopt;
  }

  const int visto = observa_relogio(cama.punho());
  if (visto < 0) {
    if (razao) {
      *razao = std::string("mpv_observe_property: ") +
               mpv().mpv_error_string(visto);
    }
    return std::nullopt;
  }
  return MotorMpv(cama.solta());
}

namespace {

// Espera a faixa carregar. Devolve verdadeiro sómente se ella carregou de
// facto: sem esta espera, quem lesse o relogio logo apoz tocar não acharia
// propriedade alguma, e o relogio pareceria quebrado quando estava por nascer.
//
// Trocar de faixa com outra A TOCAR faz o mpv annunciar o fim da ANTERIOR
// antes de começar a nova. Por isso se espera primeiro pelo começo, e só
// depois se lê o fim como falha: sem esta distinção, toda troca de faixa em
// pleno som se daria por falhada, que é o coração do que a issue pede.
bool aguarda_carga(::mpv_handle* punho, double prazo) {
  bool comecou = false;
  for (;;) {
    ::mpv_event* evento = mpv().mpv_wait_event(punho, prazo);
    switch (evento->event_id) {
      case MPV_EVENT_START_FILE:
        comecou = true;
        break;
      case MPV_EVENT_FILE_LOADED:
        return true;
      case MPV_EVENT_END_FILE:
        if (comecou) return false;  // o fim é da faixa NOVA: falhou de facto
        break;                      // é a anterior que se despede; segue-se
      case MPV_EVENT_NONE:          // o prazo acabou
      case MPV_EVENT_SHUTDOWN:
        return false;
      default:
        break;  // os demais avisos da carga não interessam a esta espera
    }
  }
}

}  // namespace

// «replace» é o que mantem a playlist do mpv com uma entrada só: a ordem das
// faixas é NOSSA, e não d'elle.
bool MotorMpv::tocar(const std::string& caminho) {
  const char* ordem[] = {"loadfile", caminho.c_str(), "replace", nullptr};
  if (punho_ == nullptr || mpv().mpv_command(punho_, ordem) < 0) return false;

  if (!aguarda_carga(punho_, 5.0)) {
    estado_ = Estado::Parado;
    posicao_ = 0.0;
    duracao_ = 0.0;
    return false;
  }

  posicao_ = le_dobro(punho_, "time-pos");
  duracao_ = le_dobro(punho_, "duration");
  estado_ = Estado::Tocando;
  return true;
}

// A versão da interface, ou ZERO quando a libmpv não está presente: é o UNICO
// logar d'este arquivo que se chama sem motor algum, e por isso confere a taboa.
unsigned long MotorMpv::versao_da_interface() noexcept {
  const TaboaDaLibmpv* const taboa = libmpv();
  return taboa != nullptr ? taboa->mpv_client_api_version() : 0UL;
}

// Pausar e retomar são a MESMA propriedade do mpv, com bandeira contraria.
bool MotorMpv::pausar() {
  int sim = 1;
  if (punho_ == nullptr ||
      mpv().mpv_set_property(punho_, "pause", MPV_FORMAT_FLAG, &sim) < 0) {
    return false;
  }
  estado_ = Estado::Pausado;
  return true;
}

bool MotorMpv::retomar() {
  int nao = 0;
  if (punho_ == nullptr ||
      mpv().mpv_set_property(punho_, "pause", MPV_FORMAT_FLAG, &nao) < 0) {
    return false;
  }
  estado_ = Estado::Tocando;
  return true;
}

// O alvo apara-se pela duração que o mpv nos deu, e não pela que o chamador
// supõe. Ao mpv desce cadeia, que é como a sua ordem de busca fala.
bool MotorMpv::buscar(double segundos) {
  if (punho_ == nullptr) return false;
  const std::string alvo = std::to_string(aparar_busca(segundos, duracao_));
  const char* ordem[] = {"seek", alvo.c_str(), "absolute", nullptr};
  return mpv().mpv_command(punho_, ordem) >= 0;
}

// O volume do MOTOR, jamais o do systema: o do systema pertence ao vol.sh, e
// nenhuma linha d'este arquivo o nomeia.
bool MotorMpv::volume(int porcento) {
  if (punho_ == nullptr) return false;
  double valor = static_cast<double>(aparar_volume(porcento));
  return mpv().mpv_set_property(punho_, "volume", MPV_FORMAT_DOUBLE, &valor) >=
         0;
}

double MotorMpv::posicao() const { return posicao_; }
double MotorMpv::duracao() const { return duracao_; }
Estado MotorMpv::estado() const { return estado_; }

std::string MotorMpv::propriedade(const char* nome) const {
  if (punho_ == nullptr) return {};
  char* texto = mpv().mpv_get_property_string(punho_, nome);
  if (texto == nullptr) return {};
  std::string colhido(texto);
  mpv().mpv_free(texto);
  return colhido;
}

// Drena a fila de avisos do mpv e assenta o que d'ella se aprende. Devolve ao
// chamador tão logo a fila esvazie: o prazo zero é o que faz d'esta funcção uma
// batida, e não uma espera.
void MotorMpv::bombear() {
  if (punho_ == nullptr) return;
  for (;;) {
    ::mpv_event* evento = mpv().mpv_wait_event(punho_, 0.0);
    if (evento->event_id == MPV_EVENT_NONE) return;

    if (evento->event_id == MPV_EVENT_PROPERTY_CHANGE) {
      const auto* propriedade =
          static_cast<::mpv_event_property*>(evento->data);
      if (propriedade == nullptr || propriedade->data == nullptr ||
          propriedade->format != MPV_FORMAT_DOUBLE) {
        continue;
      }
      const double valor = *static_cast<double*>(propriedade->data);
      const std::string_view nome(propriedade->name);
      if (nome == "time-pos") posicao_ = valor;
      if (nome == "duration") duracao_ = valor;
    } else if (evento->event_id == MPV_EVENT_END_FILE ||
               evento->event_id == MPV_EVENT_SHUTDOWN) {
      estado_ = Estado::Parado;
      posicao_ = 0.0;
    }
  }
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
