// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LETRA — src/nucleo/letra.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o libcurl no fim, e sozinho.
//
// DOMÍNIO ......... o que se sabe da faixa, e o corpo do LRCLIB.
// CONTRA-DOMÍNIO .. um `.lrc` ao lado do audio, ou nada.
// INVARIANTE ...... funcção alguma d'aqui lança, e letra ausente não é falha.
// Q.E.D. .......... a rede toca-se n'uma funcção só, e por isso a bateria julga
//                   tudo o mais sobre corpos escriptos á mão.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/letra.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <fstream>

#include "api/jsonzinho.hpp"

namespace mysong::nucleo {

std::string escapa_para_url(std::string_view crua) {
  static const char kCifras[] = "0123456789ABCDEF";
  std::string obra;
  obra.reserve(crua.size() * 3);
  for (const unsigned char octeto : crua) {
    // A lista do que NÃO se escapa é a do RFC 3986 para «unreserved», e é
    // fechada: escapar de mais é sempre seguro, e escapar de menos parte a
    // consulta no primeiro `&` que um titulo traga.
    const bool livre = std::isalnum(octeto) != 0 || octeto == '-' ||
                       octeto == '.' || octeto == '_' || octeto == '~';
    if (livre) {
      obra += static_cast<char>(octeto);
      continue;
    }
    obra += '%';
    obra += kCifras[octeto >> 4];
    obra += kCifras[octeto & 0x0F];
  }
  return obra;
}

std::string url_da_busca(std::string_view artista, std::string_view titulo) {
  std::string url = "https://lrclib.net/api/search?track_name=";
  url += escapa_para_url(titulo);
  if (!artista.empty()) {
    url += "&artist_name=";
    url += escapa_para_url(artista);
  }
  return url;
}

std::string primeiro_objecto(std::string_view arranjo) {
  std::size_t principio = arranjo.find('{');
  if (principio == std::string_view::npos) return {};
  int fundo = 0;
  bool dentro_de_aspas = false, escapado = false;
  for (std::size_t i = principio; i < arranjo.size(); ++i) {
    const char octeto = arranjo[i];
    // A ordem d'estas tres guardas é load-bearing. O escapado consome-se antes de
    // tudo; as aspas mudam o modo; e sómente FÓRA das aspas as chaves contam. Sem
    // isto, uma letra de musica que traga `}` fecharia o objecto a meio.
    if (escapado) { escapado = false; continue; }
    if (octeto == '\\' && dentro_de_aspas) { escapado = true; continue; }
    if (octeto == '"') { dentro_de_aspas = !dentro_de_aspas; continue; }
    if (dentro_de_aspas) continue;
    if (octeto == '{') ++fundo;
    else if (octeto == '}' && --fundo == 0)
      return std::string(arranjo.substr(principio, i - principio + 1));
  }
  return {};  // arranjo truncado: não se devolve objecto meio, devolve-se nada
}

std::filesystem::path caminho_do_lrc(const std::filesystem::path& audio) {
  std::filesystem::path lrc = audio;
  lrc.replace_extension(".lrc");
  return lrc;
}

Letra le_resposta(std::string_view corpo) {
  Letra letra;
  const std::string objecto = primeiro_objecto(corpo);
  if (objecto.empty()) return letra;
  // O jsonzinho d'esta Casa é o mesmo que o socket da issue #4 usa: objecto PLANO
  // de um nivel, com valores escalares. É exactamente a fórma que o LRCLIB dá.
  const api::Mensagem lida = api::analysa(objecto);
  if (!lida.valida) return letra;
  const api::Valor* sincronizada = lida.acha("syncedLyrics");
  const api::Valor* plana = lida.acha("plainLyrics");
  if (sincronizada != nullptr &&
      sincronizada->typo == api::Typo::Texto)
    letra.sincronizada = sincronizada->texto;
  if (plana != nullptr && plana->typo == api::Typo::Texto)
    letra.plana = plana->texto;
  return letra;
}

bool grava_lrc(const std::filesystem::path& audio, const Letra& letra) {
  if (letra.sincronizada.empty()) return false;
  const std::filesystem::path onde = caminho_do_lrc(audio);
  std::ofstream sahida(onde, std::ios::binary | std::ios::trunc);
  if (!sahida) return false;
  sahida << letra.sincronizada;
  if (letra.sincronizada.back() != '\n') sahida << '\n';
  return sahida.good();
}

namespace {

// recolhe — o que o libcurl entrega, pedaço a pedaço. Assignatura fixada por elle.
std::size_t recolhe(char* pedaco, std::size_t largura, std::size_t quantos,
                    void* alvo) {
  const std::size_t medida = largura * quantos;
  static_cast<std::string*>(alvo)->append(pedaco, medida);
  return medida;
}

}  // namespace

bool busca_letra(std::string_view artista, std::string_view titulo,
                 Letra* letra) {
  if (titulo.empty()) return false;
  CURL* punho = curl_easy_init();
  if (punho == nullptr) return false;
  const std::string url = url_da_busca(artista, titulo);
  std::string corpo;
  curl_easy_setopt(punho, CURLOPT_URL, url.c_str());
  curl_easy_setopt(punho, CURLOPT_WRITEFUNCTION, recolhe);
  curl_easy_setopt(punho, CURLOPT_WRITEDATA, &corpo);
  curl_easy_setopt(punho, CURLOPT_FOLLOWLOCATION, 1L);
  // Oito segundos, e não sem prazo: quem baixa uma faixa não ha de esperar por um
  // serviço de letra mais do que isso, e serviço mudo pendura o download inteiro.
  curl_easy_setopt(punho, CURLOPT_TIMEOUT, 8L);
  curl_easy_setopt(punho, CURLOPT_USERAGENT, "mysong/0.1 (+github.com/bragaus/mysong)");
  const CURLcode desfecho = curl_easy_perform(punho);
  curl_easy_cleanup(punho);
  if (desfecho != CURLE_OK) return false;
  if (letra != nullptr) *letra = le_resposta(corpo);
  return true;
}

namespace {

// carimbo — lê `[mm:ss.cc]` ou `[mm:ss]` no principio do que resta. Devolve falso
// quando não ha carimbo alli, e ahi `cursor` não se mexe.
bool carimbo(std::string_view linha, std::size_t* cursor, double* tempo) {
  std::size_t i = *cursor;
  if (i >= linha.size() || linha[i] != '[') return false;
  ++i;
  const std::size_t principio_min = i;
  while (i < linha.size() && std::isdigit(static_cast<unsigned char>(linha[i]))) ++i;
  if (i == principio_min || i >= linha.size() || linha[i] != ':') return false;
  const int minutos = std::atoi(std::string(linha.substr(principio_min, i - principio_min)).c_str());
  ++i;
  const std::size_t principio_seg = i;
  while (i < linha.size() &&
         (std::isdigit(static_cast<unsigned char>(linha[i])) || linha[i] == '.' ||
          linha[i] == ':'))
    ++i;
  if (i == principio_seg || i >= linha.size() || linha[i] != ']') return false;
  // O separador dos centesimos é ponto ou DOUS PONTOS: o fórmato admitte os dous,
  // e ha gerador que usa o segundo. Troca-se antes de converter.
  std::string segundos(linha.substr(principio_seg, i - principio_seg));
  for (char& letra : segundos) if (letra == ':') letra = '.';
  *tempo = minutos * 60.0 + std::atof(segundos.c_str());
  *cursor = i + 1;
  return true;
}

}  // namespace

std::vector<LinhaDaLetra> analysa_lrc(std::string_view texto) {
  std::vector<LinhaDaLetra> linhas;
  std::size_t principio = 0;
  while (principio <= texto.size()) {
    std::size_t fim = texto.find('\n', principio);
    if (fim == std::string_view::npos) fim = texto.size();
    std::string_view linha = texto.substr(principio, fim - principio);
    if (!linha.empty() && linha.back() == '\r') linha.remove_suffix(1);
    principio = fim + 1;

    // Todos os carimbos da frente, e sómente depois o texto: o mesmo verso pode
    // trazer varios tempos, e cada um d'elles gera a sua linha.
    std::vector<double> tempos;
    std::size_t cursor = 0;
    double tempo = 0.0;
    while (carimbo(linha, &cursor, &tempo)) tempos.push_back(tempo);
    if (tempos.empty()) continue;  // linha sem carimbo: cabeçalho ou lixo

    std::string corpo(linha.substr(cursor));
    // Apara-se sómente o espaço da FRENTE: o do fim pode ser intencional em letra
    // que se alinhe, e tirá-lo não melhora cousa alguma.
    std::size_t desde = 0;
    while (desde < corpo.size() &&
           std::isspace(static_cast<unsigned char>(corpo[desde])) != 0)
      ++desde;
    corpo.erase(0, desde);
    for (const double quando : tempos) linhas.push_back({quando, corpo});
    if (fim == texto.size()) break;
  }
  std::stable_sort(linhas.begin(), linhas.end(),
                   [](const LinhaDaLetra& a, const LinhaDaLetra& b) {
                     return a.tempo < b.tempo;
                   });
  return linhas;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
