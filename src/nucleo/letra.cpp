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

#include <cctype>
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

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
