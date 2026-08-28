// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MPRIS — src/api/mpris.cpp
// ══════════════════════════════════════════════════════════════════════════
// O encanamento da `libdbus`. A traducção de unidades NÃO vive aqui: vive em
// `unidades.cpp`, e é lá que ella se prova.
//
// DOMÍNIO ......... mensagens do barramento de sessão.
// CONTRA-DOMÍNIO .. respostas, e o pregão `PropertiesChanged`.
// INVARIANTE ...... mensagem desconhecida recebe ERRO NOMEADO, e nunca silencio:
//                   cliente que espera resposta e não a recebe fica pendurado no
//                   seu proprio prazo, e o operador vê o playerctl a travar sem
//                   razão dita.
// Q.E.D. .......... barramento ausente devolve uma Casa morta com razão, e o
//                   tocador segue: é o mesmo padrão do analisador da issue #5.
// ══════════════════════════════════════════════════════════════════════════
#include "api/mpris.hpp"

#include <dbus/dbus.h>

#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

#include "api/unidades.hpp"

namespace mysong::api {

namespace {

constexpr const char* kNome = "org.mpris.MediaPlayer2.mysong";
constexpr const char* kCaminho = "/org/mpris/MediaPlayer2";
constexpr const char* kRaiz = "org.mpris.MediaPlayer2";
constexpr const char* kTocador = "org.mpris.MediaPlayer2.Player";
constexpr const char* kPropriedades = "org.freedesktop.DBus.Properties";


// ── OS ESCRIPTORES de valores. Um por typo, para que a fórma se escreva UMA vez.

void escreve_texto(DBusMessageIter* pae, const std::string& valor) {
  const char* cru = valor.c_str();
  dbus_message_iter_append_basic(pae, DBUS_TYPE_STRING, &cru);
}

// escreve_variante — o valor embrulhado n'uma variante, que é o que o `Get` devolve.
// A assignatura entra como cadeia porque a `libdbus` a pede assim.
void escreve_variante_texto(DBusMessageIter* pae, const std::string& valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "s", &dentro);
  escreve_texto(&dentro, valor);
  dbus_message_iter_close_container(pae, &dentro);
}

void escreve_variante_caminho(DBusMessageIter* pae, const std::string& valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "o", &dentro);
  const char* cru = valor.c_str();
  dbus_message_iter_append_basic(&dentro, DBUS_TYPE_OBJECT_PATH, &cru);
  dbus_message_iter_close_container(pae, &dentro);
}

void escreve_variante_int64(DBusMessageIter* pae, std::int64_t valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "x", &dentro);
  dbus_int64_t cru = valor;
  dbus_message_iter_append_basic(&dentro, DBUS_TYPE_INT64, &cru);
  dbus_message_iter_close_container(pae, &dentro);
}

void escreve_variante_duplo(DBusMessageIter* pae, double valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "d", &dentro);
  dbus_message_iter_append_basic(&dentro, DBUS_TYPE_DOUBLE, &valor);
  dbus_message_iter_close_container(pae, &dentro);
}

void escreve_variante_bool(DBusMessageIter* pae, bool valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "b", &dentro);
  dbus_bool_t cru = valor ? TRUE : FALSE;
  dbus_message_iter_append_basic(&dentro, DBUS_TYPE_BOOLEAN, &cru);
  dbus_message_iter_close_container(pae, &dentro);
}


// escreve_metadados — o `a{sv}` do `Metadata`. Quatro chaves, e as quatro que o
// `playerctl metadata` mostra por defeito.
void escreve_metadados(DBusMessageIter* pae, const nucleo::Tocador& tocador) {
  DBusMessageIter variante, mapa;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "a{sv}", &variante);
  dbus_message_iter_open_container(&variante, DBUS_TYPE_ARRAY, "{sv}", &mapa);

  // De UMA tomada da tranca: faixa, indice e duração do mesmo momento.
  const nucleo::Retracto agora = tocador.retracto();
  const bool ha = agora.tamanho > 0;
  const std::string& caminho = agora.faixa;

  const auto par = [&mapa](const char* chave, auto escriptor) {
    DBusMessageIter entrada;
    dbus_message_iter_open_container(&mapa, DBUS_TYPE_DICT_ENTRY, nullptr,
                                     &entrada);
    dbus_message_iter_append_basic(&entrada, DBUS_TYPE_STRING, &chave);
    escriptor(&entrada);
    dbus_message_iter_close_container(&mapa, &entrada);
  };

  par("mpris:trackid", [&](DBusMessageIter* onde) {
    escreve_variante_caminho(onde, caminho_da_faixa(agora.indice, ha));
  });
  if (ha) {
    par("xesam:url", [&](DBusMessageIter* onde) {
      escreve_variante_texto(onde, url_do_arquivo(caminho));
    });
    par("xesam:title", [&](DBusMessageIter* onde) {
      // O titulo é o NOME do arquivo, e não o caminho: é o que o operador lê no
      // painel do systema, e o caminho inteiro alli não caberia.
      escreve_variante_texto(onde,
                             std::filesystem::path(caminho).filename().string());
    });
    par("mpris:length", [&](DBusMessageIter* onde) {
      escreve_variante_int64(onde, segundos_para_micros(agora.duracao));
    });
  }
  dbus_message_iter_close_container(&variante, &mapa);
  dbus_message_iter_close_container(pae, &variante);
}


// escreve_propriedade — UMA propriedade, pelo nome. Falso quando o nome não é d'esta
// Casa, e ahi quem chama devolve o erro nomeado que a especificação pede.
bool escreve_propriedade(DBusMessageIter* pae, const std::string& interface,
                         const std::string& nome, nucleo::Tocador& tocador) {
  if (interface == kRaiz) {
    if (nome == "Identity") { escreve_variante_texto(pae, "mysong"); return true; }
    if (nome == "DesktopEntry") { escreve_variante_texto(pae, "mysong"); return true; }
    // As tres que o playerctl consulta antes de commandar. `CanQuit` falso e
    // `CanRaise` falso são a verdade: esta Casa não sahe nem se ergue por commando de
    // fóra, e mentir alli faria o cliente pedir o que não se faz.
    if (nome == "CanQuit") { escreve_variante_bool(pae, false); return true; }
    if (nome == "CanRaise") { escreve_variante_bool(pae, false); return true; }
    if (nome == "HasTrackList") { escreve_variante_bool(pae, false); return true; }
    return false;
  }
  if (interface != kTocador) return false;
  if (nome == "PlaybackStatus") {
    escreve_variante_texto(pae, std::string(estado_do_mpris(tocador.estado())));
    return true;
  }
  if (nome == "Metadata") { escreve_metadados(pae, tocador); return true; }
  if (nome == "Position") {
    escreve_variante_int64(pae, segundos_para_micros(tocador.posicao()));
    return true;
  }
  if (nome == "Volume") {
    escreve_variante_duplo(pae, porcento_para_volume(tocador.volume()));
    return true;
  }
  // Os DOUS MODOS (issue #62). Sahem do retracto, que é UMA tomada da tranca.
  if (nome == "Shuffle") {
    escreve_variante_bool(pae, tocador.retracto().embaralhado);
    return true;
  }
  if (nome == "LoopStatus") {
    escreve_variante_texto(
        pae, std::string(repeticao_do_mpris(tocador.retracto().repeticao)));
    return true;
  }
  if (nome == "Rate" || nome == "MinimumRate" || nome == "MaximumRate") {
    escreve_variante_duplo(pae, 1.0);  // esta Casa não muda a velocidade
    return true;
  }
  // As seis capacidades. Todas verdadeiras menos a de girar a lista, que não ha.
  if (nome == "CanGoNext" || nome == "CanGoPrevious" || nome == "CanPlay" ||
      nome == "CanPause" || nome == "CanSeek" || nome == "CanControl") {
    escreve_variante_bool(pae, true);
    return true;
  }
  return false;
}


// cumpre_metodo — os oito metodos do MPRIS. Falso quando o nome não é d'esta Casa.
//
// As duas armadilhas que a issue nomeou: `Seek` é RELATIVO e `SetPosition` é
// ABSOLUTO. Trocá-los faria a tecla de avanço saltar para o segundo cinco em vez de
// avançar cinco segundos, e o defeito passaria por «funciona mal» em vez de «está
// trocado».
bool cumpre_metodo(const std::string& nome, DBusMessage* mensagem,
                   nucleo::Tocador& tocador) {
  if (nome == "Play") { tocador.retomar(); return true; }
  if (nome == "Pause") { tocador.pausar(); return true; }
  if (nome == "Stop") { tocador.pausar(); return true; }
  if (nome == "PlayPause") {
    if (tocador.estado() == nucleo::Estado::Tocando) tocador.pausar();
    else tocador.retomar();
    return true;
  }
  if (nome == "Next") { tocador.proxima(); return true; }
  if (nome == "Previous") { tocador.anterior(); return true; }

  if (nome == "Seek") {
    dbus_int64_t delta = 0;
    DBusMessageIter leitor;
    if (dbus_message_iter_init(mensagem, &leitor) &&
        dbus_message_iter_get_arg_type(&leitor) == DBUS_TYPE_INT64) {
      dbus_message_iter_get_basic(&leitor, &delta);
      // RELATIVO: soma-se á posição corrente. E o alvo apara-se aqui, que o delta
      // pode ser negativo e maior que a posição.
      double alvo = tocador.posicao() + micros_para_segundos(
                        delta < 0 ? -delta : delta) * (delta < 0 ? -1.0 : 1.0);
      if (alvo < 0.0) alvo = 0.0;
      tocador.buscar(alvo);
    }
    return true;
  }
  if (nome == "SetPosition") {
    DBusMessageIter leitor;
    if (dbus_message_iter_init(mensagem, &leitor) &&
        dbus_message_iter_get_arg_type(&leitor) == DBUS_TYPE_OBJECT_PATH) {
      // O `trackid` ignora-se de proposito: esta Casa tem UMA faixa corrente, e
      // recusar por trackid alheio faria o playerctl parecer roto sem razão.
      dbus_message_iter_next(&leitor);
      dbus_int64_t micros = 0;
      if (dbus_message_iter_get_arg_type(&leitor) == DBUS_TYPE_INT64) {
        dbus_message_iter_get_basic(&leitor, &micros);
        tocador.buscar(micros_para_segundos(micros));  // ABSOLUTO
      }
    }
    return true;
  }
  return false;
}

}  // namespace

namespace {

// A INTROSPECÇÃO. Cadeia fixa, porque o que se publica é fixo. Quem acrescentar
// metodo ha de acrescentá-lo aqui tambem, e é de proposito que estão juntos: o
// `playerctl` lê isto para saber o que pedir, e metodo que responde mas não se
// annuncia é metodo que ninguem chama.
constexpr const char* kIntrospecção =
    "<node>"
    "<interface name='org.freedesktop.DBus.Introspectable'>"
    "<method name='Introspect'><arg name='xml' type='s' direction='out'/></method>"
    "</interface>"
    "<interface name='org.freedesktop.DBus.Properties'>"
    "<method name='Get'><arg type='s' direction='in'/><arg type='s' direction='in'/>"
    "<arg type='v' direction='out'/></method>"
    "<method name='GetAll'><arg type='s' direction='in'/>"
    "<arg type='a{sv}' direction='out'/></method>"
    "<method name='Set'><arg type='s' direction='in'/><arg type='s' direction='in'/>"
    "<arg type='v' direction='in'/></method>"
    "<signal name='PropertiesChanged'><arg type='s'/><arg type='a{sv}'/>"
    "<arg type='as'/></signal>"
    "</interface>"
    "<interface name='org.mpris.MediaPlayer2'>"
    "<property name='Identity' type='s' access='read'/>"
    "<property name='DesktopEntry' type='s' access='read'/>"
    "<property name='CanQuit' type='b' access='read'/>"
    "<property name='CanRaise' type='b' access='read'/>"
    "<property name='HasTrackList' type='b' access='read'/>"
    "</interface>"
    "<interface name='org.mpris.MediaPlayer2.Player'>"
    "<method name='Play'/><method name='Pause'/><method name='PlayPause'/>"
    "<method name='Stop'/><method name='Next'/><method name='Previous'/>"
    "<method name='Seek'><arg name='Offset' type='x' direction='in'/></method>"
    "<method name='SetPosition'><arg name='TrackId' type='o' direction='in'/>"
    "<arg name='Position' type='x' direction='in'/></method>"
    "<property name='PlaybackStatus' type='s' access='read'/>"
    "<property name='Metadata' type='a{sv}' access='read'/>"
    "<property name='Position' type='x' access='read'/>"
    "<property name='Volume' type='d' access='readwrite'/>"
    "<property name='Shuffle' type='b' access='readwrite'/>"
    "<property name='LoopStatus' type='s' access='readwrite'/>"
    "<property name='CanGoNext' type='b' access='read'/>"
    "<property name='CanGoPrevious' type='b' access='read'/>"
    "<property name='CanPlay' type='b' access='read'/>"
    "<property name='CanPause' type='b' access='read'/>"
    "<property name='CanSeek' type='b' access='read'/>"
    "<property name='CanControl' type='b' access='read'/>"
    "</interface>"
    "</node>";

}  // namespace

namespace {

// responde_get_all — o `a{sv}` de TODAS as propriedades de uma interface. O
// `playerctl` chama-o antes de chamar `Get`, e uma Casa que sómente saiba `Get`
// parece muda a elle.
void responde_get_all(DBusMessage* resposta, const std::string& interface,
                      nucleo::Tocador& tocador) {
  static const char* kDaRaiz[] = {"Identity", "DesktopEntry", "CanQuit",
                                  "CanRaise", "HasTrackList"};
  static const char* kDoTocador[] = {
      "PlaybackStatus", "Metadata",   "Position",  "Volume",     "Rate",
      "MinimumRate",    "MaximumRate", "CanGoNext", "CanGoPrevious",
      "CanPlay",        "CanPause",   "CanSeek",   "CanControl",
      "Shuffle",        "LoopStatus"};

  DBusMessageIter fóra, mapa;
  dbus_message_iter_init_append(resposta, &fóra);
  dbus_message_iter_open_container(&fóra, DBUS_TYPE_ARRAY, "{sv}", &mapa);
  const bool raiz = interface == kRaiz;
  // A contagem sae do PROPRIO arranjo. Estava chumbada, e propriedade nova com o
  // numero esquecido faria o playerctl ler menos do que ha, sem erro algum.
  const std::size_t quantas = raiz ? std::size(kDaRaiz) : std::size(kDoTocador);
  for (std::size_t i = 0; i < quantas; ++i) {
    const char* nome = raiz ? kDaRaiz[i] : kDoTocador[i];
    DBusMessageIter entrada;
    dbus_message_iter_open_container(&mapa, DBUS_TYPE_DICT_ENTRY, nullptr,
                                     &entrada);
    dbus_message_iter_append_basic(&entrada, DBUS_TYPE_STRING, &nome);
    escreve_propriedade(&entrada, interface, nome, tocador);
    dbus_message_iter_close_container(&mapa, &entrada);
  }
  dbus_message_iter_close_container(&fóra, &mapa);
}

}  // namespace

namespace {

// annuncia_mudanca — o `PropertiesChanged` das cinco que podem mudar. `Position` NÃO
// entra: é decreto da especificação do MPRIS, porque ella muda a todo instante e um
// pregão por instante afogaria o barramento. Quem quer a posição chama `Get`.
void annuncia_mudanca(DBusConnection* ligacao, nucleo::Tocador& tocador) {
  DBusMessage* pregao =
      dbus_message_new_signal(kCaminho, kPropriedades, "PropertiesChanged");
  if (pregao == nullptr) return;
  DBusMessageIter fóra, mapa, vazio;
  dbus_message_iter_init_append(pregao, &fóra);
  const char* interface = kTocador;
  dbus_message_iter_append_basic(&fóra, DBUS_TYPE_STRING, &interface);
  dbus_message_iter_open_container(&fóra, DBUS_TYPE_ARRAY, "{sv}", &mapa);
  for (const char* nome :
       {"PlaybackStatus", "Metadata", "Volume", "Shuffle", "LoopStatus"}) {
    DBusMessageIter entrada;
    dbus_message_iter_open_container(&mapa, DBUS_TYPE_DICT_ENTRY, nullptr,
                                     &entrada);
    dbus_message_iter_append_basic(&entrada, DBUS_TYPE_STRING, &nome);
    escreve_propriedade(&entrada, kTocador, nome, tocador);
    dbus_message_iter_close_container(&mapa, &entrada);
  }
  dbus_message_iter_close_container(&fóra, &mapa);
  // O terceiro argumento é a lista das INVALIDADAS, e vae vazia: as tres que mudaram
  // vão com o seu valor, donde nada fica por reler. Omittir o container faria a
  // mensagem não casar com a assignatura e o barramento recusá-la.
  dbus_message_iter_open_container(&fóra, DBUS_TYPE_ARRAY, "s", &vazio);
  dbus_message_iter_close_container(&fóra, &vazio);
  dbus_connection_send(ligacao, pregao, nullptr);
  dbus_message_unref(pregao);
}

}  // namespace

struct CasaDoMpris::Punho {
  nucleo::Tocador& tocador;
  DBusConnection* ligacao = nullptr;
  std::string razao;
  // O ULTIMO retracto annunciado, para que o pregão sahia sómente quando muda. Sem
  // isto, `PropertiesChanged` sahiria vinte vezes por segundo e todo cliente do
  // barramento pagaria por ella.
  nucleo::Estado ultimo_estado = nucleo::Estado::Parado;
  int ultimo_volume = -1;
  std::string ultima_faixa = "\x01";  // valor impossivel, para forçar o primeiro
  bool ultimo_embaralhado = false;
  nucleo::Repeticao ultima_repeticao = nucleo::Repeticao::Nenhuma;

  explicit Punho(nucleo::Tocador& t) : tocador(t) {}
};

CasaDoMpris::CasaDoMpris(nucleo::Tocador& tocador)
    : punho_(std::make_unique<Punho>(tocador)) {
  DBusError erro;
  dbus_error_init(&erro);
  punho_->ligacao = dbus_bus_get(DBUS_BUS_SESSION, &erro);
  if (dbus_error_is_set(&erro)) {
    punho_->razao = erro.message != nullptr ? erro.message : "barramento mudo";
    dbus_error_free(&erro);
    punho_->ligacao = nullptr;
    return;
  }
  if (punho_->ligacao == nullptr) {
    punho_->razao = "não ha barramento de sessão";
    return;
  }
  // NÃO se sahe do processo quando a ligação cahe. O defeito é da libdbus por
  // defeito: ella chama exit() se o barramento morrer, e um tocador que morre porque
  // o D-Bus reiniciou é inaceitavel.
  dbus_connection_set_exit_on_disconnect(punho_->ligacao, FALSE);

  const int posse = dbus_bus_request_name(punho_->ligacao, kNome,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE, &erro);
  if (dbus_error_is_set(&erro)) {
    punho_->razao = erro.message != nullptr ? erro.message : "nome recusado";
    dbus_error_free(&erro);
    punho_->ligacao = nullptr;
    return;
  }
  if (posse != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
    // Outro mysong já tem o nome. NÃO se enfileira: o segundo não ha de herdar o
    // commando quando o primeiro sahir, que o operador não saberia qual commanda.
    punho_->razao = "outro mysong já publica " + std::string(kNome);
    punho_->ligacao = nullptr;
  }
}

CasaDoMpris::~CasaDoMpris() {
  if (punho_->ligacao != nullptr) {
    dbus_bus_release_name(punho_->ligacao, kNome, nullptr);
    dbus_connection_unref(punho_->ligacao);
  }
}

bool CasaDoMpris::viva() const noexcept { return punho_->ligacao != nullptr; }
const std::string& CasaDoMpris::razao() const noexcept { return punho_->razao; }

namespace {

// assenta_propriedade — o miolo do `Set`, já dentro da variante. Sae á parte para
// que cada propriedade que se ponha traga a sua guarda de typo ao lado do seu
// effeito, em vez de as guardas todas se empilharem antes do primeiro effeito.
DBusMessage* assenta_propriedade(CasaDoMpris::Punho& punho, DBusMessage* pedido,
                                 const std::string& nome,
                                 DBusMessageIter* dentro) {
  const int typo = dbus_message_iter_get_arg_type(dentro);
  if (nome == "Volume") {
    if (typo != DBUS_TYPE_DOUBLE)
      return dbus_message_new_error(pedido, DBUS_ERROR_INVALID_ARGS,
                                    "o volume é um duplo de zero a um");
    double valor = 0.0;
    dbus_message_iter_get_basic(dentro, &valor);
    punho.tocador.volume(volume_para_porcento(valor));
    return dbus_message_new_method_return(pedido);
  }
  return dbus_message_new_error(pedido, DBUS_ERROR_PROPERTY_READ_ONLY,
                                "essa propriedade não se põe");
}

// responde_propriedades — o `Get`, o `GetAll` e o `Set` da interface de propriedades.
DBusMessage* responde_propriedades(CasaDoMpris::Punho& punho, DBusMessage* pedido,
                                   const std::string& membro) {
  DBusMessageIter leitor;
  if (!dbus_message_iter_init(pedido, &leitor) ||
      dbus_message_iter_get_arg_type(&leitor) != DBUS_TYPE_STRING)
    return dbus_message_new_error(pedido, DBUS_ERROR_INVALID_ARGS,
                                  "faltou o nome da interface");
  const char* interface_crua = nullptr;
  dbus_message_iter_get_basic(&leitor, &interface_crua);
  const std::string interface(interface_crua != nullptr ? interface_crua : "");

  if (membro == "GetAll") {
    if (interface != kRaiz && interface != kTocador)
      return dbus_message_new_error(pedido, DBUS_ERROR_UNKNOWN_INTERFACE,
                                    "esta Casa não serve essa interface");
    DBusMessage* resposta = dbus_message_new_method_return(pedido);
    if (resposta != nullptr) responde_get_all(resposta, interface, punho.tocador);
    return resposta;
  }

  dbus_message_iter_next(&leitor);
  if (dbus_message_iter_get_arg_type(&leitor) != DBUS_TYPE_STRING)
    return dbus_message_new_error(pedido, DBUS_ERROR_INVALID_ARGS,
                                  "faltou o nome da propriedade");
  const char* nome_cru = nullptr;
  dbus_message_iter_get_basic(&leitor, &nome_cru);
  const std::string nome(nome_cru != nullptr ? nome_cru : "");

  if (membro == "Get") {
    DBusMessage* resposta = dbus_message_new_method_return(pedido);
    if (resposta == nullptr) return nullptr;
    DBusMessageIter fóra;
    dbus_message_iter_init_append(resposta, &fóra);
    if (escreve_propriedade(&fóra, interface, nome, punho.tocador)) return resposta;
    // Propriedade que não ha: desfaz-se a resposta meia e manda-se erro NOMEADO.
    dbus_message_unref(resposta);
    return dbus_message_new_error(pedido, DBUS_ERROR_UNKNOWN_PROPERTY,
                                  "esta Casa não tem essa propriedade");
  }

  if (membro == "Set") {
    // SÓMENTE o volume se põe, que é o unico `readwrite` da introspecção. Aceitar
    // outro faria a Casa mentir sobre o que a introspecção promette.
    if (interface != kTocador || nome != "Volume")
      return dbus_message_new_error(pedido, DBUS_ERROR_PROPERTY_READ_ONLY,
                                    "sómente o volume se põe");
    dbus_message_iter_next(&leitor);
    DBusMessageIter dentro;
    if (dbus_message_iter_get_arg_type(&leitor) != DBUS_TYPE_VARIANT)
      return dbus_message_new_error(pedido, DBUS_ERROR_INVALID_ARGS,
                                    "o valor ha de vir n'uma variante");
    dbus_message_iter_recurse(&leitor, &dentro);
    return assenta_propriedade(punho, pedido, nome, &dentro);
  }
  return dbus_message_new_error(pedido, DBUS_ERROR_UNKNOWN_METHOD,
                                "esta Casa não conhece esse metodo");
}

// responde_um — UMA mensagem. Mensagem que esta Casa não conheça recebe ERRO NOMEADO,
// e nunca silencio: cliente que espera resposta e não a recebe fica pendurado no seu
// proprio prazo, e o operador vê o playerctl a travar sem razão dita.
void responde_um(CasaDoMpris::Punho& punho, DBusMessage* pedido) {
  const char* interface_crua = dbus_message_get_interface(pedido);
  const char* membro_cru = dbus_message_get_member(pedido);
  if (interface_crua == nullptr || membro_cru == nullptr) return;
  const std::string interface(interface_crua), membro(membro_cru);
  DBusMessage* resposta = nullptr;

  if (interface == "org.freedesktop.DBus.Introspectable" && membro == "Introspect") {
    resposta = dbus_message_new_method_return(pedido);
    if (resposta != nullptr) {
      DBusMessageIter fóra;
      dbus_message_iter_init_append(resposta, &fóra);
      escreve_texto(&fóra, kIntrospecção);
    }
  } else if (interface == kPropriedades) {
    resposta = responde_propriedades(punho, pedido, membro);
  } else if (interface == kRaiz || interface == kTocador) {
    if (cumpre_metodo(membro, pedido, punho.tocador))
      resposta = dbus_message_new_method_return(pedido);
    else
      resposta = dbus_message_new_error(
          pedido, DBUS_ERROR_UNKNOWN_METHOD, "esta Casa não conhece esse metodo");
  } else {
    resposta = dbus_message_new_error(pedido, DBUS_ERROR_UNKNOWN_INTERFACE,
                                      "esta Casa não serve essa interface");
  }
  if (resposta != nullptr) {
    dbus_connection_send(punho.ligacao, resposta, nullptr);
    dbus_message_unref(resposta);
  }
}

}  // namespace

void CasaDoMpris::pulsa() {
  if (punho_->ligacao == nullptr) return;
  // NÃO bloqueia: zero de prazo. Quem chama pulsa a vinte por segundo e não ha de
  // esperar pelo barramento dentro do seu proprio laço de desenho.
  dbus_connection_read_write(punho_->ligacao, 0);

  while (DBusMessage* pedido = dbus_connection_pop_message(punho_->ligacao)) {
    responde_um(*punho_, pedido);
    dbus_message_unref(pedido);
  }

  // E o pregão, sómente quando muda. A comparação é dos CINCO: estado, volume,
  // faixa e os dous modos, colhidos de UMA tomada da tranca do tocador.
  const nucleo::Retracto agora = punho_->tocador.retracto();
  if (agora.estado != punho_->ultimo_estado ||
      agora.volume != punho_->ultimo_volume ||
      agora.faixa != punho_->ultima_faixa ||
      agora.embaralhado != punho_->ultimo_embaralhado ||
      agora.repeticao != punho_->ultima_repeticao) {
    punho_->ultimo_estado = agora.estado;
    punho_->ultimo_volume = agora.volume;
    punho_->ultima_faixa = agora.faixa;
    punho_->ultimo_embaralhado = agora.embaralhado;
    punho_->ultima_repeticao = agora.repeticao;
    annuncia_mudanca(punho_->ligacao, punho_->tocador);
  }
  dbus_connection_flush(punho_->ligacao);
}

}  // namespace mysong::api

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
