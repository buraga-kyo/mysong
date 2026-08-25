// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO CATALOGO — src/nucleo/catalogo.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. O recorte do JSON vem de api/jsonzinho.hpp, e não se repete
// aqui: a letra da issue #14 já usa o mesmo.
//
// DOMÍNIO ......... a URL, e o corpo da pagina de embutir.
// CONTRA-DOMÍNIO .. o catalogo.
// INVARIANTE ...... faixa sem titulo não sahe, e a ordem que se põe é a da LISTA e
//                   não a das que sobreviveram: a faixa tres continua a ser a tres
//                   ainda que a dous se tenha recusado.
// Q.E.D. .......... o `trackList` acha-se por nome em qualquer fundo do embrulho,
//                   donde a leitura sobrevive a o Spotify mudar de quantos niveis
//                   de embrulho o rodeia, que é cousa que elle muda.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/catalogo.hpp"

#include <curl/curl.h>

#include <cstddef>
#include <utility>

#include "api/jsonzinho.hpp"

namespace mysong::nucleo {
namespace {

std::size_t recolhe(char* pedaco, std::size_t tamanho, std::size_t quantos,
                    void* fora) {
  const std::size_t bytes = tamanho * quantos;
  static_cast<std::string*>(fora)->append(pedaco, bytes);
  return bytes;
}

// O identificador do Spotify tem vinte e dous caracteres de base sessenta e dous.
// Não se afere o comprimento: elle é convenção d'elles e pode mudar. Afere-se o
// ALPHABETO, que é o que separa identificador de pedaço de URL.
bool letra_de_id(char octeto) {
  return (octeto >= 'a' && octeto <= 'z') || (octeto >= 'A' && octeto <= 'Z') ||
         (octeto >= '0' && octeto <= '9');
}

}  // namespace

std::string id_da_playlist(std::string_view crua) {
  // Tres fórmas, e uma regra: acha-se a palavra `playlist` seguida de separador, e
  // o que vem depois d'ella até ao proximo separador é o identificador. Serve á URL
  // de `open.spotify.com`, á de embutir, e ao URI `spotify:playlist:`.
  const std::string_view agulha = "playlist";
  std::size_t onde = crua.find(agulha);
  if (onde == std::string_view::npos) return {};
  std::size_t i = onde + agulha.size();
  if (i >= crua.size() || (crua[i] != '/' && crua[i] != ':')) return {};
  ++i;
  const std::size_t principio = i;
  while (i < crua.size() && letra_de_id(crua[i])) ++i;
  return std::string(crua.substr(principio, i - principio));
}

std::string nome_da_lista(std::string_view corpo) {
  // O nome vem de FÓRA do arranjo, no mesmo objecto que o tras. Procura-se pois o
  // ULTIMO `"name"` ANTES do `"trackList"`: é o mais proximo, e é o d'ella. Contar
  // niveis de embrulho seria escolher uma versão da pagina d'elles para sempre.
  const std::size_t lista = corpo.find("\"trackList\"");
  if (lista == std::string_view::npos) return {};
  const std::string_view antes = corpo.substr(0, lista);
  const std::size_t onde = antes.rfind("\"name\"");
  if (onde == std::string_view::npos) return {};
  // Embrulha-se n'um objecto de mentira, que é a fórma que o leitor de fundo um
  // espera; e apara-se em meio kilo-octeto, que nome de lista não é maior que isso e
  // embrulhar a pagina inteira seria copiar cento e cincoenta kilo-octetos por nada.
  const std::string_view pedaco = antes.substr(onde, 512);
  return api::texto_de_chave("{" + std::string(pedaco) + "}", "name");
}

std::string url_do_embed(std::string_view id) {
  if (id.empty()) return {};
  return "https://open.spotify.com/embed/playlist/" + std::string(id);
}

Catalogo le_catalogo(std::string_view corpo) {
  Catalogo catalogo;
  // O arranjo das faixas acha-se pelo NOME da chave, em qualquer fundo: o Spotify
  // muda de quantos niveis de embrulho a rodeia, e contá-los seria escolher uma
  // versão da pagina d'elles para sempre.
  const std::string arranjo = api::recorta_arranjo(corpo, "trackList");
  if (arranjo.empty()) return catalogo;
  int ordem = 0;
  for (const std::string& objecto : api::objectos_do_arranjo(arranjo)) {
    ++ordem;  // conta-se a ORDEM DA LISTA, e não a das que sobreviveram
    FaixaDoCatalogo faixa;
    faixa.titulo = api::texto_de_chave(objecto, "title");
    // O `subtitle` é o que a pagina põe por artista, e ella junta os artistas com
    // virgula quando ha mais de um. Fica como veio: quem busca no YouTube busca
    // melhor com os dous nomes que com um só.
    faixa.artista = api::texto_de_chave(objecto, "subtitle");
    double milesimos = 0.0;
    if (api::numero_de_chave(objecto, "duration", &milesimos))
      faixa.duracao_ms = static_cast<int>(milesimos);
    faixa.numero = ordem;
    // Faixa sem TITULO não sahe: ella seria linha que o operador elege e que não se
    // pode buscar, que é buscar por cadeia vazia.
    if (faixa.titulo.empty()) continue;
    catalogo.faixas.push_back(std::move(faixa));
  }
  catalogo.nome = nome_da_lista(corpo);
  return catalogo;
}

bool busca_catalogo(std::string_view crua, Catalogo* catalogo) {
  const std::string id = id_da_playlist(crua);
  if (id.empty()) return false;
  CURL* punho = curl_easy_init();
  if (punho == nullptr) return false;
  const std::string url = url_do_embed(id);
  std::string corpo;
  curl_easy_setopt(punho, CURLOPT_URL, url.c_str());
  curl_easy_setopt(punho, CURLOPT_WRITEFUNCTION, recolhe);
  curl_easy_setopt(punho, CURLOPT_WRITEDATA, &corpo);
  curl_easy_setopt(punho, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(punho, CURLOPT_TIMEOUT, 15L);
  // O Spotify recusa quem não se nomeie por navegador. Diz-se o que se é DEPOIS do
  // que elle exige: mentir sobre o navegador é o preço de a pagina ser servida, e
  // o nome d'esta obra vae no fim para que o registro d'elles saiba quem pediu.
  curl_easy_setopt(punho, CURLOPT_USERAGENT,
                   "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                   "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 "
                   "mysong/0.1");
  const CURLcode desfecho = curl_easy_perform(punho);
  curl_easy_cleanup(punho);
  if (desfecho != CURLE_OK) return false;
  if (catalogo != nullptr) *catalogo = le_catalogo(corpo);
  return true;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
