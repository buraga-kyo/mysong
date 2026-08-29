// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MUSICBRAINZ — src/nucleo/musicbrainz.hpp
// ══════════════════════════════════════════════════════════════════════════
// A GRAVAÇÃO por traz da faixa (issue #57). O catalogo da issue #13 casava o
// audio por titulo e duração, crivo que acceita cover e versão ao vivo; o que
// separa a gravação certa de toda imitação é o ISRC, e quem o dá de graça é o
// MusicBrainz: dados CC0, API ws/2 sem chave, sem conta e sem credencial
// alguma. Foi essa a ordem d'esta obra, e é essa a fronteira.
//
// O CAMINHO: o id do track (que a pagina de embutir já tras) acha a gravação
// pela relação de URL; não havendo o link, a busca por artista, titulo e
// duração; e a ficha da gravação dá ISRC, album, anno, numero e duração exacta.
//
// DOMÍNIO ......... o id de um track do Spotify, ou artista+titulo+duração; e
//                   os corpos JSON que o ws/2 devolve.
// CONTRA-DOMÍNIO .. uma FichaMB, ou o aviso honesto de que não se achou.
// INVARIANTE ...... NUNCA mais de uma requisição por segundo, somados todos os
//                   fios, e toda requisição se nomeia pelo agente d'esta obra:
//                   são as duas regras publicadas do servidor, e quebrá-las é
//                   ser recusado (503) em nome de todos os que pedem d'este IP.
// Q.E.D. .......... sendo puras a construcção das consultas e a leitura das
//                   respostas, a bateria afere-as contra corpos colhidos da
//                   API viva em 2026-08-27, sem tocar a rede.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// O AGENTE. O MusicBrainz exige `nome/versão (contato)` e recusa o anonymo.
inline constexpr char kAgenteDoMB[] =
    "mysong/0.1 (https://github.com/bragaus/mysong)";

// A TOLERANCIA do casamento por duração, em SEGUNDOS, e a FONTE do numero
// (issue #63). Doze: o mesmo audio costuma trazer um ou dous segundos de
// silencio nas pontas, e a versão ao vivo ou a estendida differe de muito mais
// que isso. Mora AQUI, e não na aquisição, por razão de dependencia e não de
// gosto: `aquisicao.hpp` inclue este cabeçalho, e o contrario seria cyclo.
inline constexpr int kToleranciaSeg = 12;

// A MESMA tolerancia em milesimos, que é como o MusicBrainz fala de duração.
// DERIVA, e não se declara: quem mudar os doze muda UM logar.
inline constexpr int kJanellaMs = kToleranciaSeg * 1000;

// A FICHA de uma gravação. Campo vazio ou zero é «o MusicBrainz não disse».
struct FichaMB {
  std::vector<std::string> isrcs;  // na ordem do MB; uma gravação accumula varios
  std::string titulo;
  std::string artista;
  std::string album;   // da release canonica: Official, grupo Album puro, a mais antiga
  int ano = 0;         // o anno d'essa release
  int numero = 0;      // a posição da faixa n'ella (`position`; `number` vem «A3»)
  int duracao_ms = 0;  // a duração exacta da gravação, em milesimos
};

// escapa_url — o percent-encoding do RFC 3986, proprio porque o do curl pede
// punho vivo. Sómente letra, cifra e `-._~` passam crus.
std::string escapa_url(std::string_view crua);

// url_da_consulta_pelo_link — a relação de URL do track do Spotify: casamento
// EXACTO quando o MusicBrainz tem o link. Vazio sem identificador.
std::string url_da_consulta_pelo_link(std::string_view id_do_track);

// url_da_consulta_pela_busca — gravação por artista, titulo e duração (janella
// de doze segundos, a da tolerancia do casamento). Vazio sem titulo; sem
// artista ou sem duração, a clausula que falta fica de fóra.
std::string url_da_consulta_pela_busca(std::string_view artista,
                                       std::string_view titulo, int duracao_ms);

// url_da_ficha — a gravação inteira por MBID: ISRCs, artistas e releases com
// grupo e numeração. Vazio sem MBID.
std::string url_da_ficha(std::string_view mbid);

// le_gravacao_da_url — o MBID da gravação que o url-lookup aponta. Vazio quando
// o corpo não tras relação de gravação (404, truncado, alheio).
std::string le_gravacao_da_url(std::string_view corpo);

// le_ficha_da_gravacao — a ficha que o corpo do lookup tras: ISRCs na ordem do
// MB, titulo, artista, duração exacta, e a release canonica (album, anno,
// numero). Campo que o corpo não diga fica vazio ou zero.
FichaMB le_ficha_da_gravacao(std::string_view corpo);

// le_eleita_da_busca — o MBID eleito do corpo da busca: score ≥ 90, duração
// dentro da janella quando o catalogo a disse, e a de first-release-date mais
// antiga entre as que passam. Vazio quando nenhuma passa.
std::string le_eleita_da_busca(std::string_view corpo, int duracao_ms);

// O TECTO de ISRCs que a colheita tenta por faixa (RULINGS R5). Tres: um só
// degradaria faixa cujo primeiro ISRC o YouTube não indexou (medido), e sem
// tecto uma ficha de oito ISRCs custaria oito buscas de rede.
inline constexpr std::size_t kIsrcsPorFaixa = 3;

// termos_de_busca — os termos com que a colheita busca o YouTube, NESTA ordem:
// até kIsrcsPorFaixa ISRCs da ficha, e por derradeiro o termo de hoje (artista
// e titulo), que é o ultimo recurso e sahe com a duvida confessada.
std::vector<std::string> termos_de_busca(const FichaMB& ficha,
                                         const std::string& artista,
                                         const std::string& titulo);

// ── E AGORA O QUE TOCA O MUNDO ──────────────────────────────────────────────

// espera_a_vez_do_mb — o acelerador: segura o fio até haver um segundo inteiro
// desde a ultima passagem, somados todos os fios do processo. Publico para que
// a bateria o afira com relogio, sem rede alguma.
void espera_a_vez_do_mb();

// O DESFECHO de uma consulta. TRES, e não um booleano: o 404 e a rede muda
// mandam ao caminho seguinte, mas o 503 e o 429 mandam PARAR, que gastar a
// consulta seguinte contra quem pediu recuo engrossa a rajada que o acelerador
// existe para impedir.
enum class DesfechoMB {
  Achado,  // 2xx, e o corpo está no logar
  Falhou,  // 404, outra recusa, 5xx que não peça recuo, ou rede muda
  Recuo,   // 429 ou 503: o servidor pediu menos trafego
};

// desfecho_da_resposta — a leitura do que a rede devolveu, PURA, para que a
// bateria a afira sem rede. `erro_do_curl` é o codigo do libcurl (zero é o
// «correu bem» d'elle), e `estado` é o codigo HTTP.
DesfechoMB desfecho_da_resposta(int erro_do_curl, long estado);

// consulta_mb — pede a URL com o agente da obra, passando pelo acelerador, e
// enche o corpo. Re-tento algum se faz aqui: uma fila de faixas a re-tentar
// amplificaria a rajada que o acelerador impede.
DesfechoMB consulta_mb(const std::string& url, std::string* corpo);

// A CONSULTA por parametro: é a junta do dublê, a mesma do Estaleiro. Entrando
// a rede por parametro, a bateria põe no logar d'ella uma consulta de mentira e
// afere QUANTAS e QUAES consultas a resolução gasta, sem tocar a rede.
using Consulta = std::function<DesfechoMB(const std::string&, std::string*)>;

// resolve_gravacao — a resolução inteira: pelo link do track quando o ha, pela
// busca quando não; e a ficha da gravação eleita. Duração em MILESIMOS, como o
// MB fala. Falso quando nada casou, e ahi quem chama confessa a duvida. Pedindo
// o servidor RECUO no primeiro caminho, o segundo NÃO se gasta.
bool resolve_gravacao(const std::string& id_spotify, const std::string& artista,
                      const std::string& titulo, int duracao_ms, FichaMB* ficha,
                      const Consulta& consulta = consulta_mb);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
