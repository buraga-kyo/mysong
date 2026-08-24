// ══════════════════════════════════════════════════════════════════════════
//   A CARNE DO ANALISADOR — src/nucleo/analisador.cpp
// ══════════════════════════════════════════════════════════════════════════
// O TRACTADO vive no cabeçalho. Aqui mora o PipeWire, e sómente aqui: é esta a
// unica unidade de traducção da Casa que inclue pipewire.h.
//
// Duas linhas de execução se cruzam n'este arquivo, e é bom sabê-lo antes de o
// ler: o callback de processo corre na linha de TEMPO REAL do PipeWire, e
// bandas() com pulsa() correm na linha de quem chama. A fechadura guarda o
// espectro com o retracto, e nada mais: quem a tomasse para mais tempo pagaria
// em falha de audio.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/analisador.hpp"

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <set>

#include "nucleo/espectro.hpp"

namespace mysong::nucleo {
namespace {
using Relogio = std::chrono::steady_clock;

// Quantos millesimos se passaram entre dous instantes.
double millesimos_entre(Relogio::time_point antes, Relogio::time_point depois) {
  return std::chrono::duration<double, std::milli>(depois - antes).count();
}

}  // namespace

// O PUNHO: todo o PipeWire d'esta Casa cabe aqui dentro, e nada d'elle sahe pelo
// cabeçalho. Os callbacks são methodos estaticos por necessidade: elles hão de
// nomear este typo, que é privado do Analisador, e função livre não poderia.
struct Analisador::Punho {
  ::pw_thread_loop* laco = nullptr;
  ::pw_context* contexto = nullptr;
  ::pw_core* nucleo = nullptr;
  ::pw_registry* registro = nullptr;
  ::pw_stream* fluxo = nullptr;
  ::spa_hook ouvido_do_registro {};
  ::spa_hook ouvido_do_fluxo {};

  mutable std::mutex boca;
  Espectro espectro;
  std::vector<float> retracto = std::vector<float>(QUANTAS_BANDAS, 0.0f);
  Relogio::time_point ultimo_buffer = Relogio::now();
  Relogio::time_point ultimo_pulso = Relogio::now();

  // O CENSO do grafo, que é o que permitte eleger sem depender da ordem de
  // chegada. Os clientes do NOSSO processo, e todos os nós de fluxo de sahida
  // com o cliente que os possue e o serial que os nomeia.
  std::set<std::uint32_t> nossos_clientes;
  std::map<std::uint32_t, std::uint32_t> nos;
  std::map<std::uint32_t, unsigned long long> seriaes;

  std::uint32_t no_preso = SPA_ID_INVALID;
  unsigned long long serial_preso = 0;
  std::uint32_t nosso_pid = 0;
  std::string razao;

  // A eleição, e o que d'ella decorre.
  void elege();
  void prende(unsigned long long serial);
  void solta();
  void engole(const float* amostras, std::size_t quantas);

  // Os quatro callbacks que o PipeWire chama, e as duas taboadas que o ligam a
  // elles. As taboadas nascem em estatico de função, por assignação campo a
  // campo: inicializador designado é cousa de C, e sob -Wall -Wextra em C++ elle
  // cobraria aviso de campo que falta, que é o nosso zero por terra.
  static void em_global(void* dados, std::uint32_t id, std::uint32_t permissoes,
                        const char* tipo, std::uint32_t versao,
                        const ::spa_dict* props);
  static void em_global_removido(void* dados, std::uint32_t id);
  static void em_processo(void* dados);
  static void em_formato(void* dados, std::uint32_t id, const ::spa_pod* param);
  static const ::pw_registry_events& eventos_do_registro();
  static const ::pw_stream_events& eventos_do_fluxo();
};

void Analisador::Punho::em_global(void* dados, std::uint32_t id, std::uint32_t,
                                 const char* tipo, std::uint32_t,
                                 const ::spa_dict* props) {
  auto* eu = static_cast<Punho*>(dados);
  if (eu == nullptr || tipo == nullptr || props == nullptr) return;

  if (std::strcmp(tipo, PW_TYPE_INTERFACE_Client) == 0) {
    // O NOSSO cliente: aquelle cujo processo é o nosso. A libmpv toca DENTRO do
    // nosso processo, donde o cliente que ella abre no PipeWire traz o nosso
    // proprio pid. É por aqui que a identidade entra, e não pelo nome «mpv»,
    // que na machina de quem ouve musica ha muitos.
    //
    // O pid vem do pipewire.sec.pid, e não do application.process.id. Duas
    // razões, e a primeira é dura: o annuncio do registro NÃO carrega o
    // application.process.id (medido: carrega sómente serial, modulo,
    // protocolo, os quatro pipewire.sec e o nome), donde procurá-lo ali é achar
    // nada e nunca prender nó algum. A segunda é de confiança: o sec.pid é
    // posto pelo SERVIDOR, das credenciaes do socket, e cliente algum o pode
    // mentir; o application.process.id é o cliente que o declara de si.
    const char* pid = spa_dict_lookup(props, PW_KEY_SEC_PID);
    if (pid == nullptr) pid = spa_dict_lookup(props, PW_KEY_APP_PROCESS_ID);
    if (pid != nullptr &&
        std::strtoul(pid, nullptr, 10) == static_cast<unsigned long>(eu->nosso_pid)) {
      eu->nossos_clientes.insert(id);
    }
  } else if (std::strcmp(tipo, PW_TYPE_INTERFACE_Node) == 0) {
    const char* classe = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    const char* cliente = spa_dict_lookup(props, PW_KEY_CLIENT_ID);
    const char* serial = spa_dict_lookup(props, PW_KEY_OBJECT_SERIAL);
    if (classe == nullptr || cliente == nullptr) return;
    if (std::strcmp(classe, "Stream/Output/Audio") != 0) return;
    eu->nos[id] = static_cast<std::uint32_t>(std::strtoul(cliente, nullptr, 10));
    eu->seriaes[id] = serial != nullptr ? std::strtoull(serial, nullptr, 10) : 0;
  } else {
    return;
  }

  // Relege a CADA annuncio, e não sómente quando chega um nó: o registro pode
  // annunciar o nó ANTES do cliente que o possue, e quem elegesse uma vez só
  // perderia o nosso nó por ordem de chegada. É a ordem do mundo, que dublê
  // nenhum tem.
  eu->elege();
}

void Analisador::Punho::em_global_removido(void* dados, std::uint32_t id) {
  auto* eu = static_cast<Punho*>(dados);
  if (eu == nullptr) return;
  eu->nossos_clientes.erase(id);
  eu->nos.erase(id);
  eu->seriaes.erase(id);
  // Cliente que sahe leva os seus nós. Sem esta varredura, o nó de um cliente
  // morto ficaria na taboada para sempre, e a taboada crescia a cada faixa: o nó
  // do mpv nasce e morre com cada uma d'ellas.
  for (auto it = eu->nos.begin(); it != eu->nos.end();) {
    if (it->second == id) {
      eu->seriaes.erase(it->first);
      it = eu->nos.erase(it);
    } else {
      ++it;
    }
  }
  eu->elege();
}

void Analisador::Punho::elege() {
  // O de SERIAL MAIOR entre os nossos, que é o mais novo. Na troca de faixa o
  // mpv ergue o nó novo ANTES de matar o velho, donde ha um instante com dous
  // nós nossos no grafo; tomar o mais novo é tomar a faixa que começa, e não a
  // que acaba.
  std::uint32_t melhor = SPA_ID_INVALID;
  unsigned long long maior = 0;
  for (const auto& par : nos) {
    if (nossos_clientes.count(par.second) == 0) continue;
    const auto achado = seriaes.find(par.first);
    const unsigned long long serial = achado != seriaes.end() ? achado->second : 0;
    if (melhor == SPA_ID_INVALID || serial > maior) {
      melhor = par.first;
      maior = serial;
    }
  }
  if (melhor == no_preso) return;
  solta();
  no_preso = melhor;
  serial_preso = melhor != SPA_ID_INVALID ? maior : 0;
  if (melhor != SPA_ID_INVALID) prende(serial_preso);
}

void Analisador::Punho::solta() {
  if (fluxo == nullptr) return;
  pw_stream_destroy(fluxo);
  fluxo = nullptr;
  // O ouvido morre com o fluxo que o pendurava: destruir o fluxo já o desliga,
  // e removê-lo de novo seria mexer em lista que já não existe.
  ouvido_do_fluxo = ::spa_hook {};
}

const ::pw_stream_events& Analisador::Punho::eventos_do_fluxo() {
  static ::pw_stream_events taboada = [] {
    ::pw_stream_events feitos {};
    feitos.version = PW_VERSION_STREAM_EVENTS;
    feitos.param_changed = &Punho::em_formato;
    feitos.process = &Punho::em_processo;
    return feitos;
  }();
  return taboada;
}

const ::pw_registry_events& Analisador::Punho::eventos_do_registro() {
  static ::pw_registry_events taboada = [] {
    ::pw_registry_events feitos {};
    feitos.version = PW_VERSION_REGISTRY_EVENTS;
    feitos.global = &Punho::em_global;
    feitos.global_remove = &Punho::em_global_removido;
    return feitos;
  }();
  return taboada;
}

void Analisador::Punho::prende(unsigned long long serial) {
  if (nucleo == nullptr) return;
  char alvo[32];
  std::snprintf(alvo, sizeof(alvo), "%llu", serial);

  // O ALVO desce pelo object.serial, e NUNCA pelo object.id. Medido n'esta
  // machina: com o id, o gestor de sessão IGNORA o pedido e liga-nos ao
  // MICROPHONE, que também se move com a musica (por vasamento acustico do fone
  // para o microphone) e portanto passa na prova ingenua e falha na do aceite.
  // Foi o defeito que mais perto passou de entrar n'esta obra.
  ::pw_properties* props = pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture",
      PW_KEY_MEDIA_ROLE, "Music", PW_KEY_NODE_NAME, "mysong-analisador",
      PW_KEY_TARGET_OBJECT, alvo, PW_KEY_STREAM_CAPTURE_SINK, "false", nullptr);
  fluxo = pw_stream_new(nucleo, "mysong-analisador", props);
  if (fluxo == nullptr) return;
  pw_stream_add_listener(fluxo, &ouvido_do_fluxo, &eventos_do_fluxo(), this);

  // Pede-se F32 em 48000 e dous canaes, e o adaptador do PipeWire converte o que
  // o nó tiver: elle negociou S16LE n'este teste, e chegou F32 aqui. É por essa
  // conversão de graça que taxa differente e fluxo mono não pedem codigo nosso,
  // e é o param_changed que diz o que de facto veio.
  std::uint8_t espaco[1024];
  ::spa_pod_builder construtor = SPA_POD_BUILDER_INIT(espaco, sizeof(espaco));
  ::spa_audio_info_raw crua {};
  crua.format = SPA_AUDIO_FORMAT_F32;
  crua.rate = 48000;
  crua.channels = 2;
  crua.position[0] = SPA_AUDIO_CHANNEL_FL;
  crua.position[1] = SPA_AUDIO_CHANNEL_FR;
  const ::spa_pod* params[1] = {
      spa_format_audio_raw_build(&construtor, SPA_PARAM_EnumFormat, &crua)};
  pw_stream_connect(fluxo, PW_DIRECTION_INPUT, PW_ID_ANY,
                      static_cast<::pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                                     PW_STREAM_FLAG_MAP_BUFFERS |
                                                     PW_STREAM_FLAG_RT_PROCESS),
                      params, 1);
}

void Analisador::Punho::em_formato(void* dados, std::uint32_t id,
                                   const ::spa_pod* param) {
  auto* eu = static_cast<Punho*>(dados);
  if (eu == nullptr || param == nullptr || id != SPA_PARAM_Format) return;
  std::uint32_t meio = 0;
  std::uint32_t submeio = 0;
  if (spa_format_parse(param, &meio, &submeio) < 0) return;
  if (meio != SPA_MEDIA_TYPE_audio || submeio != SPA_MEDIA_SUBTYPE_raw) return;
  ::spa_audio_info_raw crua {};
  if (spa_format_audio_raw_parse(param, &crua) < 0) return;
  // A taxa com os canaes vêm do formato CONFIRMADO, e não do que se pediu: pedir
  // não é receber, e a conta que presumisse 48000 poria as bandas no logar
  // errado em qualquer placa que negociasse outra cousa.
  std::lock_guard<std::mutex> tranca(eu->boca);
  eu->espectro.assenta_formato(static_cast<float>(crua.rate),
                               static_cast<int>(crua.channels));
}

void Analisador::Punho::engole(const float* amostras, std::size_t quantas) {
  std::lock_guard<std::mutex> tranca(boca);
  espectro.alimenta(amostras, quantas);
  retracto = espectro.bandas();
  ultimo_buffer = Relogio::now();
}

void Analisador::Punho::em_processo(void* dados) {
  auto* eu = static_cast<Punho*>(dados);
  if (eu == nullptr || eu->fluxo == nullptr) return;
  ::pw_buffer* pedaco = pw_stream_dequeue_buffer(eu->fluxo);
  if (pedaco == nullptr) return;  // sem buffer prompto não é erro: é espera
  const ::spa_buffer* buffer = pedaco->buffer;
  if (buffer != nullptr && buffer->n_datas > 0 && buffer->datas[0].data != nullptr &&
      buffer->datas[0].chunk != nullptr) {
    // O DESVIO importa: o PipeWire pode entregar as amostras adiante do inicio
    // da memoria mappeada, e ler do inicio daria silencio ou ruido de outra
    // volta do annel. Nada se presume: nem o tamanho, nem o começo.
    const auto* base = static_cast<const std::uint8_t*>(buffer->datas[0].data);
    const std::uint32_t desvio = buffer->datas[0].chunk->offset;
    const std::uint32_t bytes = buffer->datas[0].chunk->size;
    const auto* amostras = reinterpret_cast<const float*>(base + desvio);
    eu->engole(amostras, bytes / sizeof(float));
  }
  pw_stream_queue_buffer(eu->fluxo, pedaco);
}

Analisador::Analisador() : punho_(new Punho) {
  punho_->nosso_pid = static_cast<std::uint32_t>(::getpid());
  pw_init(nullptr, nullptr);

  punho_->laco = pw_thread_loop_new("mysong-analisador", nullptr);
  if (punho_->laco == nullptr) {
    punho_->razao = "não erguí a linha de execução do PipeWire";
    return;
  }
  punho_->contexto =
      pw_context_new(pw_thread_loop_get_loop(punho_->laco), nullptr, 0);
  if (punho_->contexto == nullptr) {
    punho_->razao = "não erguí o contexto do PipeWire";
    return;
  }
  // Aqui, e sómente aqui, o PipeWire pode faltar de todo: serviço morto, socket
  // ausente, sessão sem audio. Nascer INERTE é o contracto, e não excepção:
  // bandas em zero, razão legivel, e o resto do programa a correr igual.
  punho_->nucleo = pw_context_connect(punho_->contexto, nullptr, 0);
  if (punho_->nucleo == nullptr) {
    punho_->razao = "o PipeWire não respondeu: as bandas ficam em zero";
    return;
  }
  punho_->registro = pw_core_get_registry(punho_->nucleo, PW_VERSION_REGISTRY, 0);
  if (punho_->registro == nullptr) {
    punho_->razao = "não abri o registro do PipeWire";
    return;
  }
  // O ouvido se pendura ANTES de a linha começar a correr: pendurá-lo depois
  // seria correr a chance de perder o annuncio do nó que já existia.
  pw_registry_add_listener(punho_->registro, &punho_->ouvido_do_registro,
                             &Punho::eventos_do_registro(), punho_.get());
  pw_thread_loop_start(punho_->laco);
}

Analisador::~Analisador() {
  // A ORDEM é a inversa da que se ergueu, e a linha de execução para PRIMEIRO:
  // destruir o fluxo com a linha a correr seria destruí-lo debaixo do callback
  // que n'esse instante o está a usar.
  if (punho_->laco != nullptr) pw_thread_loop_stop(punho_->laco);
  punho_->solta();
  if (punho_->registro != nullptr) {
    pw_proxy_destroy(reinterpret_cast<::pw_proxy*>(punho_->registro));
  }
  if (punho_->nucleo != nullptr) pw_core_disconnect(punho_->nucleo);
  if (punho_->contexto != nullptr) pw_context_destroy(punho_->contexto);
  if (punho_->laco != nullptr) pw_thread_loop_destroy(punho_->laco);
  pw_deinit();
}

bool Analisador::vivo() const noexcept { return punho_->nucleo != nullptr; }

const std::string& Analisador::razao() const noexcept { return punho_->razao; }

unsigned long long Analisador::no() const noexcept { return punho_->serial_preso; }

std::vector<float> Analisador::bandas() const {
  // Devolve COPIA, e nunca ponteiro para dentro do que a outra linha escreve.
  // Ponteiro seria o defeito que apparece longe da causa: a barra pintaria meio
  // retracto velho com meio novo, e uma vez por hora.
  std::lock_guard<std::mutex> tranca(punho_->boca);
  return punho_->retracto;
}

void Analisador::pulsa() {
  const auto agora = Relogio::now();
  std::lock_guard<std::mutex> tranca(punho_->boca);
  // O passo se mede SEMPRE, e não sómente no silencio: medi-lo só quando ha
  // silencio daria, na primeira batida silenciosa, o tempo inteiro desde a
  // ultima, e a barra despencaria de uma vez em vez de esmorecer.
  const double passo = millesimos_entre(punho_->ultimo_pulso, agora);
  punho_->ultimo_pulso = agora;
  if (millesimos_entre(punho_->ultimo_buffer, agora) < PRAZO_DE_SILENCIO_MS) return;
  // Nó morto, ou faixa acabada, ou mpv em espera. As tres cousas se parecem d'
  // aqui, e as tres pedem a mesma resposta: esmorecer até zero, pelo tempo de
  // queda, e não saltar a zero, que na tela se lê como falha do programa.
  punho_->espectro.esmorece(passo);
  punho_->retracto = punho_->espectro.bandas();
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
