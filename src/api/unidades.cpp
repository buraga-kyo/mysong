// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS UNIDADES — src/api/unidades.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Arquivo algum d'aqui inclue `dbus/dbus.h`, e é essa a promessa:
// a traducção prova-se em machina surda.
//
// DOMÍNIO ......... escalares dos dous lados.
// CONTRA-DOMÍNIO .. escalares aparados.
// INVARIANTE ...... funcção alguma lança, e nenhuma devolve fóra do arco.
// Q.E.D. .......... `grep "#include.*dbus"` neste arquivo sahe VAZIO, e é o que faz a
//                   promessa verificavel em vez de dita. Escrevi primeiro «grep dbus»,
//                   e essa affirmação era falsa: o proprio commentario tras a palavra.
//                   Affirmação sobre um grep ha de ser o grep que se corre.
// ══════════════════════════════════════════════════════════════════════════
#include "api/unidades.hpp"

#include <cctype>
#include <cmath>

namespace mysong::api {

std::int64_t segundos_para_micros(double segundos) {
  if (!std::isfinite(segundos) || segundos <= 0.0) return 0;
  // Trunca por `static_cast`, que é o que a linguagem faz de inteiro: microssegundo a
  // mais faria o cliente pedir posição que a faixa já não tem.
  return static_cast<std::int64_t>(segundos * static_cast<double>(kMicrosPorSegundo));
}

double micros_para_segundos(std::int64_t micros) {
  if (micros <= 0) return 0.0;
  return static_cast<double>(micros) / static_cast<double>(kMicrosPorSegundo);
}

double porcento_para_volume(int porcento) {
  if (porcento <= 0) return 0.0;
  if (porcento >= 100) return 1.0;
  return static_cast<double>(porcento) / 100.0;
}

int volume_para_porcento(double volume) {
  if (!std::isfinite(volume) || volume <= 0.0) return 0;
  if (volume >= 1.0) return 100;
  return static_cast<int>(std::lround(volume * 100.0));
}

std::string_view estado_do_mpris(nucleo::Estado estado) {
  switch (estado) {
    case nucleo::Estado::Tocando: return "Playing";
    case nucleo::Estado::Pausado: return "Paused";
    case nucleo::Estado::Parado: break;
  }
  return "Stopped";
}

std::string caminho_da_faixa(std::size_t indice, bool ha_faixa) {
  // O `NoTrack` da especificação, e não um caminho inventado.
  if (!ha_faixa) return "/org/mpris/MediaPlayer2/TrackList/NoTrack";
  return "/br/us/braga/mysong/faixa/" + std::to_string(indice);
}

std::string url_do_arquivo(std::string_view caminho) {
  static const char kCifras[] = "0123456789ABCDEF";
  std::string url = "file://";
  for (const unsigned char octeto : caminho) {
    // A BARRA passa: ella é a estructura do caminho, e escapá-la faria a URL deixar
    // de nomear um arquivo. O resto do arco livre é o do RFC 3986.
    const bool livre = std::isalnum(octeto) != 0 || octeto == '/' ||
                       octeto == '-' || octeto == '.' || octeto == '_' ||
                       octeto == '~';
    if (livre) {
      url += static_cast<char>(octeto);
      continue;
    }
    url += '%';
    url += kCifras[octeto >> 4];
    url += kCifras[octeto & 0x0F];
  }
  return url;
}

}  // namespace mysong::api

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
