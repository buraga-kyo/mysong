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
    const char* pid = ::spa_dict_lookup(props, PW_KEY_APP_PROCESS_ID);
    if (pid != nullptr &&
        std::strtoul(pid, nullptr, 10) == static_cast<unsigned long>(eu->nosso_pid)) {
      eu->nossos_clientes.insert(id);
    }
  } else if (std::strcmp(tipo, PW_TYPE_INTERFACE_Node) == 0) {
    const char* classe = ::spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    const char* cliente = ::spa_dict_lookup(props, PW_KEY_CLIENT_ID);
    const char* serial = ::spa_dict_lookup(props, PW_KEY_OBJECT_SERIAL);
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
  ::pw_stream_destroy(fluxo);
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
  ::pw_properties* props = ::pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture",
      PW_KEY_MEDIA_ROLE, "Music", PW_KEY_NODE_NAME, "mysong-analisador",
      PW_KEY_TARGET_OBJECT, alvo, PW_KEY_STREAM_CAPTURE_SINK, "false", nullptr);
  fluxo = ::pw_stream_new(nucleo, "mysong-analisador", props);
  if (fluxo == nullptr) return;
  ::pw_stream_add_listener(fluxo, &ouvido_do_fluxo, &eventos_do_fluxo(), this);

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
      ::spa_format_audio_raw_build(&construtor, SPA_PARAM_EnumFormat, &crua)};
  ::pw_stream_connect(fluxo, PW_DIRECTION_INPUT, PW_ID_ANY,
                      static_cast<::pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                                     PW_STREAM_FLAG_MAP_BUFFERS |
                                                     PW_STREAM_FLAG_RT_PROCESS),
                      params, 1);
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
