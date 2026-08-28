// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MUSICBRAINZ — src/nucleo/musicbrainz.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro (consultas e leituras), e a rede sozinha
// no fim, á maneira do catalogo: o que se prova está acima, o que não se prova
// está abaixo, e o olho vê a fronteira de um relance.
//
// DOMÍNIO ......... o id de um track, ou artista+titulo+duração; e os corpos.
// CONTRA-DOMÍNIO .. as URLs de consulta, e a ficha da gravação.
// INVARIANTE ...... o acelerador é UM para o processo inteiro: dous obreiros da
//                   fila de baixa somam UMA requisição por segundo, e não duas.
// Q.E.D. .......... as fixtures da bateria são recortes VERBATIM das respostas
//                   vivas de 2026-08-27; leitor que as lê, lê a API de verdade.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/musicbrainz.hpp"

#include <curl/curl.h>

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <mutex>
#include <string_view>
#include <thread>

#include "api/jsonzinho.hpp"

namespace mysong::nucleo {
namespace {

// A raiz do ws/2. N'uma constante para que as tres consultas digam UM endereço.
constexpr char kRaiz[] = "https://musicbrainz.org/ws/2/";

// O TECTO do corpo de resposta: quatro mega-octetos. As respostas medidas têm
// dezenas de kilo-octetos; corpo maior é anomalia, e devolver menos que o
// pedido faz o curl abortar a transferencia em vez de a engolir inteira.
constexpr std::size_t kTectoDoCorpo = std::size_t{4} << 20;

std::size_t recolhe(char* pedaco, std::size_t tamanho, std::size_t quantos,
                    void* fora) {
  auto* corpo = static_cast<std::string*>(fora);
  const std::size_t bytes = tamanho * quantos;
  if (corpo->size() + bytes > kTectoDoCorpo) return 0;
  corpo->append(pedaco, bytes);
  return bytes;
}

// A janella da busca por duração, em milesimos: os MESMOS doze segundos da
// TOLERANCIA_DO_CASAMENTO, e pela mesma medida (silencio nas pontas fica dentro,
// versão ao vivo e estendida ficam fóra).
constexpr int kJanellaMs = 12000;

// aspas_seguras — o texto dentro de aspas da consulta Lucene. Aspa e
// contra-barra escapam-se; sem isto, um titulo com aspa partiria a frase e o
// resto do titulo viraria operador de busca.
std::string aspas_seguras(std::string_view crua) {
  std::string obra;
  obra.reserve(crua.size() + 4);
  for (const char letra : crua) {
    if (letra == '"' || letra == '\\') obra += '\\';
    obra += letra;
  }
  return obra;
}

}  // namespace

std::string escapa_url(std::string_view crua) {
  // O percent-encoding do RFC 3986, byte a byte. PROPRIO, e não o do curl: o
  // curl_easy_escape pede punho vivo, e punho em funcção pura é rede dentro do
  // que a bateria havia de provar sem rede.
  static constexpr char kHexa[] = "0123456789ABCDEF";
  std::string obra;
  obra.reserve(crua.size() * 3);
  for (const char bruto : crua) {
    const unsigned char byte = static_cast<unsigned char>(bruto);
    const bool livre = (byte >= 'A' && byte <= 'Z') ||
                       (byte >= 'a' && byte <= 'z') ||
                       (byte >= '0' && byte <= '9') || byte == '-' ||
                       byte == '.' || byte == '_' || byte == '~';
    if (livre) { obra += bruto; continue; }
    obra += '%';
    obra += kHexa[byte >> 4];
    obra += kHexa[byte & 0x0F];
  }
  return obra;
}

std::string url_da_consulta_pelo_link(std::string_view id_do_track) {
  // A relação de URL: o MusicBrainz guarda o endereço do track e a gravação a
  // que elle aponta. MEDIDO em 2026-08-27: 200 com a relação quando o link está
  // lá, e 404 quando não está; o 404 é resposta, e manda quem chama á busca.
  if (id_do_track.empty()) return {};
  const std::string track =
      "https://open.spotify.com/track/" + std::string(id_do_track);
  return std::string(kRaiz) + "url?resource=" + escapa_url(track) +
         "&inc=recording-rels&fmt=json";
}

std::string url_da_consulta_pela_busca(std::string_view artista,
                                       std::string_view titulo, int duracao_ms) {
  // O caminho segundo: gravação por artista, titulo e duração. Duração zero
  // («não disse») busca sem janella; e a de baixo apara-se no zero, que
  // janella negativa o Lucene recusa inteira.
  if (titulo.empty()) return {};
  std::string consulta = "recording:\"" + aspas_seguras(titulo) + "\"";
  if (!artista.empty())
    consulta += " AND artist:\"" + aspas_seguras(artista) + "\"";
  if (duracao_ms > 0) {
    const int piso = duracao_ms > kJanellaMs ? duracao_ms - kJanellaMs : 0;
    consulta += " AND dur:[" + std::to_string(piso) + " TO " +
                std::to_string(duracao_ms + kJanellaMs) + "]";
  }
  return std::string(kRaiz) + "recording?query=" + escapa_url(consulta) +
         "&limit=8&fmt=json";
}

std::string url_da_ficha(std::string_view mbid) {
  // A gravação inteira: ISRCs, artistas, releases com grupo e com a numeração
  // das faixas. O mbid vem do proprio MB, e ainda assim não se interpola cru.
  if (mbid.empty()) return {};
  return std::string(kRaiz) + "recording/" + escapa_url(mbid) +
         "?inc=isrcs+artist-credits+releases+release-groups+media&fmt=json";
}

std::string le_gravacao_da_url(std::string_view corpo) {
  // O corpo do url-lookup: `relations[]`, e em cada relação a gravação a que o
  // link aponta. Toma-se a PRIMEIRA cujo alvo é gravação; havendo mais de uma,
  // qualquer d'ellas é casamento que o proprio MB declarou. Corpo de 404,
  // truncado ou alheio dá vazio, que é resposta e não erro.
  const std::string arranjo = api::recorta_arranjo(corpo, "relations");
  if (arranjo.empty()) return {};
  for (const std::string& relacao : api::objectos_do_arranjo(arranjo)) {
    if (api::texto_de_chave(relacao, "target-type") != "recording") continue;
    const std::string gravacao = api::recorta_objecto(relacao, "recording");
    if (gravacao.empty()) continue;
    const std::string mbid = api::texto_de_chave(gravacao, "id");
    if (!mbid.empty()) return mbid;
  }
  return {};
}

FichaMB le_ficha_da_gravacao(std::string_view corpo) {
  // O corpo do lookup da gravação. Titulo e duração são os de fundo UM: os das
  // releases moram mais fundo, e os leitores da Casa não os confundem. O
  // credito de artista toma-se do primeiro arranjo `artist-credit` do corpo,
  // que numa gravação é o d'ella; e a ficha é «melhor esforço»: campo que o
  // corpo não traga fica vazio, e quem chama decide o que fazer com o vazio.
  FichaMB ficha;
  ficha.titulo = api::texto_de_chave(corpo, "title");
  double valor = 0.0;
  // A duração aceita-se até um dia: acima d'isso não é duração de faixa, e o
  // numero absurdo estouraria as contas de quem a consome.
  if (api::numero_de_chave(corpo, "length", &valor) && valor > 0 &&
      valor <= 86400000.0)
    ficha.duracao_ms = static_cast<int>(valor);
  ficha.isrcs = api::textos_do_arranjo(api::recorta_arranjo(corpo, "isrcs"));
  // O credito do artista lê-se com as releases EXCISADAS do corpo. O recorte
  // acha a PRIMEIRA chave do texto, em qualquer fundo; o MB serializa as chaves
  // em ordem que muda de resposta para resposta (medido: as releases á frente);
  // e cada release tras o credito PROPRIO, que n'uma compilação é «Various
  // Artists» e não o artista da gravação. Sem a excisão, o artista sahia da
  // sorte da ordem.
  const std::string releases = api::recorta_arranjo(corpo, "releases");
  std::string sem_releases(corpo);
  const std::size_t onde =
      releases.empty() ? std::string::npos : sem_releases.find(releases);
  if (onde != std::string::npos) sem_releases.erase(onde, releases.size());
  const std::vector<std::string> creditos =
      api::objectos_do_arranjo(api::recorta_arranjo(sem_releases, "artist-credit"));
  if (!creditos.empty()) ficha.artista = api::texto_de_chave(creditos[0], "name");

  // A RELEASE CANONICA (RULINGS R4): Official, de grupo «Album» SEM typo
  // secundario (compilação é Album de typo secundario), a de data mais antiga;
  // não havendo nenhuma assim, a mais antiga de qualquer feitio. MEDIDO na
  // gravação de 1987: sem a regra inteira, a mais antiga crua é um single de
  // sete pollegadas, e o «Album» mais antigo cru é uma compilação.
  std::string eleita, data_da_eleita, qualquer, data_de_qualquer;
  for (const std::string& release : api::objectos_do_arranjo(releases)) {
    const std::string data = api::texto_de_chave(release, "date");
    const std::string chave = data.empty() ? "9999" : data;  // sem data perde
    const std::string grupo = api::recorta_objecto(release, "release-group");
    const bool canonica =
        api::texto_de_chave(release, "status") == "Official" &&
        api::texto_de_chave(grupo, "primary-type") == "Album" &&
        api::textos_do_arranjo(api::recorta_arranjo(grupo, "secondary-types"))
            .empty();
    if (canonica && (eleita.empty() || chave < data_da_eleita)) {
      eleita = release;
      data_da_eleita = chave;
    }
    if (qualquer.empty() || chave < data_de_qualquer) {
      qualquer = release;
      data_de_qualquer = chave;
    }
  }
  const std::string& da_vez = !eleita.empty() ? eleita : qualquer;
  if (da_vez.empty()) return ficha;
  ficha.album = api::texto_de_chave(da_vez, "title");
  const std::string data = api::texto_de_chave(da_vez, "date");
  if (data.size() >= 4) ficha.ano = std::atoi(data.substr(0, 4).c_str());
  // O numero é `position` da faixa na primeira midia, que é inteiro; o
  // `number` impresso vem «A3» no vinil, e não serve á etiqueta.
  const std::vector<std::string> midias =
      api::objectos_do_arranjo(api::recorta_arranjo(da_vez, "media"));
  if (!midias.empty()) {
    const std::vector<std::string> faixas =
        api::objectos_do_arranjo(api::recorta_arranjo(midias[0], "tracks"));
    if (!faixas.empty() && api::numero_de_chave(faixas[0], "position", &valor))
      ficha.numero = static_cast<int>(valor);
  }
  return ficha;
}

std::string le_eleita_da_busca(std::string_view corpo, int duracao_ms) {
  // As candidatas da busca vêm quasi todas com score cem (MEDIDO: oito
  // homonymas, de 1987 a 2026), donde o score sozinho não elege. As regras
  // (RULINGS R7): score de noventa para cima; duração dentro da janella quando
  // o catalogo a disse (candidata sem duração não se afere, e não passa); e
  // entre as que passam, a de first-release-date mais antiga, que é a gravação
  // ORIGINAL que o Aceite da issue pede contra o cover e o remaster.
  std::string eleita, data_da_eleita;
  for (const std::string& gravacao :
       api::objectos_do_arranjo(api::recorta_arranjo(corpo, "recordings"))) {
    const std::string id = api::texto_de_chave(gravacao, "id");
    if (id.empty()) continue;
    double valor = 0.0;
    if (!api::numero_de_chave(gravacao, "score", &valor) || valor < 90) continue;
    if (duracao_ms > 0) {
      if (!api::numero_de_chave(gravacao, "length", &valor)) continue;
      const int longe = static_cast<int>(valor) > duracao_ms
                            ? static_cast<int>(valor) - duracao_ms
                            : duracao_ms - static_cast<int>(valor);
      if (longe > kJanellaMs) continue;
    }
    const std::string data = api::texto_de_chave(gravacao, "first-release-date");
    const std::string chave = data.empty() ? "9999" : data;
    if (eleita.empty() || chave < data_da_eleita) {
      eleita = id;
      data_da_eleita = chave;
    }
  }
  return eleita;
}

std::vector<std::string> termos_de_busca(const FichaMB& ficha,
                                         const std::string& artista,
                                         const std::string& titulo) {
  // Os ISRCs primeiro, no tecto e na ordem do MB: uma gravação accumula ISRCs
  // de remasters, e nem todo está indexado no YouTube (MEDIDO: o de 1987 não
  // acha nada, o de 2021 acha a art track da mesma gravação — audio egual).
  std::vector<std::string> termos;
  for (const std::string& isrc : ficha.isrcs) {
    if (termos.size() >= kIsrcsPorFaixa) break;
    if (!isrc.empty()) termos.push_back(isrc);
  }
  termos.push_back(artista.empty() ? titulo : artista + " " + titulo);
  return termos;
}

// ── E AGORA O QUE TOCA O MUNDO. D'aqui para baixo não ha prova de bateria que
// valha, fóra a do proprio acelerador, que toca relogio e não rede. ──────────

void espera_a_vez_do_mb() {
  // O ACELERADOR: uma requisição por segundo, somados TODOS os fios, que os
  // dous obreiros do estaleiro chegam juntos em rajada. A espera dorme com a
  // tranca tomada DE PROPOSITO: é isso que serializa a vez; e tranca alguma se
  // toma aqui dentro, donde ciclo de trancas não ha por onde nascer.
  static std::mutex tranca;
  static std::chrono::steady_clock::time_point ultima;
  std::lock_guard<std::mutex> chave(tranca);
  const auto agora = std::chrono::steady_clock::now();
  if (ultima.time_since_epoch().count() != 0 &&
      agora - ultima < std::chrono::seconds(1))
    std::this_thread::sleep_for(std::chrono::seconds(1) - (agora - ultima));
  ultima = std::chrono::steady_clock::now();
}

bool consulta_mb(const std::string& url, std::string* corpo) {
  if (url.empty() || corpo == nullptr) return false;
  espera_a_vez_do_mb();
  CURL* punho = curl_easy_init();
  if (punho == nullptr) return false;
  curl_easy_setopt(punho, CURLOPT_URL, url.c_str());
  curl_easy_setopt(punho, CURLOPT_WRITEFUNCTION, recolhe);
  curl_easy_setopt(punho, CURLOPT_WRITEDATA, corpo);
  curl_easy_setopt(punho, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(punho, CURLOPT_TIMEOUT, 15L);
  curl_easy_setopt(punho, CURLOPT_USERAGENT, kAgenteDoMB);
  const CURLcode desfecho = curl_easy_perform(punho);
  long estado = 0;
  curl_easy_getinfo(punho, CURLINFO_RESPONSE_CODE, &estado);
  curl_easy_cleanup(punho);
  // Sómente o 2xx se lê. O 404 é «não temos» e o 503 é «devagar»: ambos mandam
  // quem chama ao caminho seguinte, e NENHUM se re-tenta aqui, que uma fila de
  // faixas re-tentando amplificaria a rajada que o acelerador impede.
  return desfecho == CURLE_OK && estado >= 200 && estado < 300;
}

bool resolve_gravacao(const std::string& id_spotify, const std::string& artista,
                      const std::string& titulo, int duracao_ms,
                      FichaMB* ficha) {
  // Os dous caminhos com rede, na ordem da issue: o LINK primeiro, que é
  // casamento que o proprio MB declarou; a BUSCA depois. Consulta que falhe
  // (404, 503, rede morta, corpo alheio) cae ao passo seguinte, e o falso
  // final é o aviso honesto de que não se achou: quem chama confessa a
  // duvida em vez de baixar cousa errada calada.
  std::string mbid;
  if (!id_spotify.empty()) {
    std::string corpo;
    if (consulta_mb(url_da_consulta_pelo_link(id_spotify), &corpo))
      mbid = le_gravacao_da_url(corpo);
  }
  if (mbid.empty() && !titulo.empty()) {
    std::string corpo;
    if (consulta_mb(url_da_consulta_pela_busca(artista, titulo, duracao_ms),
                    &corpo))
      mbid = le_eleita_da_busca(corpo, duracao_ms);
  }
  if (mbid.empty()) return false;
  std::string corpo;
  if (!consulta_mb(url_da_ficha(mbid), &corpo)) return false;
  FichaMB lida = le_ficha_da_gravacao(corpo);
  if (lida.titulo.empty()) return false;  // ficha sem titulo não é gravação
  if (ficha != nullptr) *ficha = std::move(lida);
  return true;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
